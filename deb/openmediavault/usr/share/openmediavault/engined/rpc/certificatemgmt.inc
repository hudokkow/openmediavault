<?php
/**
 * This file is part of OpenMediaVault.
 *
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2016 Volker Theile
 *
 * OpenMediaVault is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OpenMediaVault is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenMediaVault. If not, see <http://www.gnu.org/licenses/>.
 */
require_once("openmediavault/globals.inc");

class OMVRpcServiceCertificateMgmt extends \OMV\Rpc\ServiceAbstract {
	/**
	 * Get the RPC service name.
	 */
	public function getName() {
		return "CertificateMgmt";
	}

	/**
	 * Initialize the RPC service.
	 */
	public function initialize() {
		$this->registerMethod("getList");
		$this->registerMethod("get");
		$this->registerMethod("set");
		$this->registerMethod("delete");
		$this->registerMethod("getDetail");
		$this->registerMethod("create");
		$this->registerMethod("getSshList");
		$this->registerMethod("createSsh");
		$this->registerMethod("getSsh");
		$this->registerMethod("setSsh");
		$this->registerMethod("deleteSsh");
	}

	/**
	 * Get list of SSL certificate configuration objects.
	 * @param params An array containing the following fields:
	 *   \em start The index where to start.
	 *   \em limit The number of objects to process.
	 *   \em sortfield The name of the column used to sort.
	 *   \em sortdir The sort direction, ASC or DESC.
	 * @param context The context of the caller.
	 * @return An array containing the requested objects. The field \em total
	 *   contains the total number of objects, \em data contains the object
	 *   array. An exception will be thrown in case of an error.
	 */
	public function getList($params, $context) {
		// Validate the RPC caller context.
		$this->validateMethodContext($context, array(
			  "role" => OMV_ROLE_ADMINISTRATOR
		  ));
		// Validate the parameters of the RPC service method.
		$this->validateMethodParams($params, "rpc.common.getlist");
		// Get list of SSL certificate configuration objects.
		$db = \OMV\Config\Database::getInstance();
		$objects = $db->get("conf.system.certificate.ssl");
		// Process the list of objects.
		$result = [];
		foreach ($objects as $objectk => $objectv) {
			// Add additional properties.
			$objectv->add("name", "string", "");
			$objectv->add("validto", "integer", 0);
			$objectv->add("_used", "boolean", FALSE);
			// Remove the private key. It should not leave the system.
			$objectv->remove("privatekey");
			// Parse the certificate.
			if ($certinfo = openssl_x509_parse($objectv->get("certificate"))) {
				$objectv->set("name", $certinfo['name']);
				$objectv->set("validto", $certinfo['validTo_time_t']);
			}
			// Set '_used' flag if the certificate is referenced.
			$objectv->set("_used", $db->isReferenced($objectv));
			// Get the values.
			$result[] = $objectv->getAssoc();
		}
		// Filter the result.
		return $this->applyFilter($result, $params['start'], $params['limit'],
		  $params['sortfield'], $params['sortdir']);
	}

	/**
	 * Get a SSL certificate configuration object.
	 * @param params An array containing the following fields:
	 *   \em uuid The UUID of the configuration object.
	 * @param context The context of the caller.
	 * @return The requested configuration object.
	 */
	function get($params, $context) {
		// Validate the RPC caller context.
		$this->validateMethodContext($context, array(
			  "role" => OMV_ROLE_ADMINISTRATOR
		  ));
		// Validate the parameters of the RPC service method.
		$this->validateMethodParams($params, "rpc.common.objectuuid");
		// Get the configuration object.
		$db = \OMV\Config\Database::getInstance();
		$object = $db->get("conf.system.certificate.ssl", $params['uuid']);
		// Remove the private key. It should not leave the system.
		$object->remove("privatekey");
		// Return the values.
		return $object->getAssoc();
	}

	/**
	 * Set (add/update) a SSL certificate configuration object
	 * @param params The method parameters.
	 * @param context The context of the caller.
	 * @return The stored configuration object.
	 */
	function set($params, $context) {
		// Validate the RPC caller context.
		$this->validateMethodContext($context, array(
			  "role" => OMV_ROLE_ADMINISTRATOR
		  ));
		// Validate the parameters of the RPC service method.
		$this->validateMethodParams($params, "rpc.certificatemgmt.set");
		// Prepare the configuration object.
		$object = new \OMV\Config\ConfigObject("conf.system.certificate.ssl");
		$object->setAssoc($params);
		// Validate the certificate.
		if (!($certificate = openssl_x509_read($object->get("certificate")))) {
			throw new \OMV\Exception("Invalid certificate: %s",
			  openssl_error_string());
		}
		// Validate the private key.
		if (!$object->isEmpty("privatekey")) {
			if (!($privatekey = openssl_pkey_get_private(
			  $object->get("privatekey")))) {
				openssl_x509_free($certificate);
				throw new \OMV\Exception("Invalid private key: %s",
				  openssl_error_string());
			}
			// Check if the private key corresponds to the certificate.
			if (!openssl_x509_check_private_key($certificate, $privatekey)) {
				openssl_pkey_free($privatekey);
				openssl_x509_free($certificate);
				throw new \OMV\Exception(
				  "Private key does not correspond to the certificate: %s",
				  openssl_error_string());
			}
			openssl_pkey_free($privatekey);
		}
		openssl_x509_free($certificate);
		// Set the configuration object.
		$isNew = $object->isNew();
		if (TRUE === $isNew) {
			// The private key is required here.
			if ($object->isEmpty("privatekey"))
				throw new \OMV\Exception("Private key does not exist.");
		}
		$db = \OMV\Config\Database::getInstance();
		$db->set($object);
		// Notify configuration changes.
		$dispatcher = \OMV\Engine\Notify\Dispatcher::getInstance();
		$dispatcher->notify($isNew ? OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
		  "org.openmediavault.conf.system.certificate.ssl",
		  $object->getAssoc());
		// Return the configuration object.
		return $object->getAssoc();
	}

	/**
	 * Delete a SSL certificate config object.
	 * @param params An array containing the following fields:
	 *   \em uuid The UUID of the configuration object.
	 * @param context The context of the caller.
	 * @return The deleted configuration object.
	 */
	public function delete($params, $context) {
		// Validate the RPC caller context.
		$this->validateMethodContext($context, array(
			  "role" => OMV_ROLE_ADMINISTRATOR
		  ));
		// Validate the parameters of the RPC service method.
		$this->validateMethodParams($params, "rpc.common.objectuuid");
		// Check if the certificate is referenced.
		$db = \OMV\Config\Database::getInstance();
		$object = $db->get("conf.system.certificate.ssl", $params['uuid']);
		$db->assertIsNotReferenced($object);
		// Delete the configuration object.
		return $this->deleteConfigObjectByObject($object,
		  "org.openmediavault.conf.system.certificate.ssl");
	}

	/**
	 * Get detail about a SSL certificate.
	 * @param params An array containing the following fields:
	 *   \em uuid The UUID of the configuration object.
	 * @param context The context of the caller.
	 * @return A string containing the certificate details.
	 * @throw \OMV\Config\ConfigDirtyException
	 */
	public function getDetail($params, $context) {
		// Validate the RPC caller context.
		$this->validateMethodContext($context, array(
			  "role" => OMV_ROLE_ADMINISTRATOR
		  ));
		// Validate the parameters of the RPC service method.
		$this->validateMethodParams($params, "rpc.common.objectuuid");
		// Check if the module is marked as dirty. This is an indication
		// that the certificate has not been created or updated until now.
		if ($this->isModuleDirty("certificatemgmt"))
			throw new \OMV\Config\ConfigDirtyException();
		// Get certificate details.
		$cmdArgs = [];
		$cmdArgs[] = "x509";
		$cmdArgs[] = "-text";
		$cmdArgs[] = "-noout";
		$cmdArgs[] = "-in";
		$cmdArgs[] = sprintf("%s/certs/%s%s.crt",
		  $GLOBALS['OMV_SSL_CERTIFICATE_DIR'],
		  $GLOBALS['OMV_SSL_CERTIFICATE_PREFIX'],
		  $params['uuid']);
		$cmd = new \OMV\System\Process("openssl", $cmdArgs);
		$cmd->setRedirect2to1(TRUE);
		$cmd->execute($output);
		return implode("\n", $output);
	}

	/**
	 * Create a SSL certificate.
	 * @param params An array containing the following fields:
	 *   \em size The size.
	 *   \em days The number of days the certificate is valid for.
	 *   \em c Country
	 *   \em st State or province
	 *   \em l Locality/city
	 *   \em o Organization name
	 *   \em ou Organization unit name
	 *   \em cn Common name
	 *   \em email Email address
	 * @param context The context of the caller.
	 * @return The configuration object.
	 */
	public function create($params, $context) {
		// Validate the RPC caller context.
		$this->validateMethodContext($context, array(
			  "role" => OMV_ROLE_ADMINISTRATOR
		  ));
		// Validate the parameters of the RPC service method.
		$this->validateMethodParams($params, "rpc.certificatemgmt.create");
		// Generate the certificate subject string.
		$rdnfields = array("c", "st", "l", "o", "ou", "cn");
		$subject = "";
		foreach ($rdnfields as $rdnfieldk => $rdnfieldv) {
			if (empty($params[$rdnfieldv]))
				continue;
			// Escape RDN attribute value, see
			// http://www-03.ibm.com/systems/i/software/ldap/underdn.html
			// http://msdn.microsoft.com/en-us/library/windows/desktop/aa366101(v=vs.85).aspx
			$replace = [];
			foreach (array(',','+','<','>',';','\r','=','#','/') as $v) {
				$replace[$v] = sprintf("\\%s", strtoupper(str_pad(dechex(
				  ord($v)), 2, "0")));
			}
			$params[$rdnfieldv] = strtr($params[$rdnfieldv], $replace);
			$params[$rdnfieldv] = addcslashes($params[$rdnfieldv], '"\\');
			// Append to subject.
			switch ($rdnfieldv) {
			case "cn":
				// Append the email address.
				if (!empty($params['email'])) {
					$params[$rdnfieldv] = sprintf("%s/emailAddress=%s",
					  $params[$rdnfieldv], $params['email']);
				}
			default:
				$subject .= sprintf("/%s=%s", strtoupper($rdnfieldv),
				  $params[$rdnfieldv]);
			}
		}
		// Create the requested certificate.
		// http://www.zytrax.com/tech/survival/ssl.html
		$keyfile = tempnam(sys_get_temp_dir(), "cert");
		$crtfile = tempnam(sys_get_temp_dir(), "cert");
		$cmdArgs = [];
		$cmdArgs[] = "req";
		$cmdArgs[] = "-x509";
		$cmdArgs[] = "-nodes";
		$cmdArgs[] = sprintf("-days %d", $params['days']);
		$cmdArgs[] = "-sha256";
		$cmdArgs[] = "-batch";
		$cmdArgs[] = sprintf("-newkey rsa:%d", $params['size']);
		$cmdArgs[] = sprintf("-keyout %s", escapeshellarg($keyfile));
		$cmdArgs[] = sprintf("-out %s", escapeshellarg($crtfile));
		if (!empty($subject))
			$cmdArgs[] = sprintf("-subj %s", escapeshellarg($subject));
		$cmd = new \OMV\System\Process("openssl", $cmdArgs);
		$cmd->setRedirect2to1(TRUE);
		$cmd->setQuiet(TRUE);
		$cmd->execute($output, $exitStatus);
		if (0 !== $exitStatus) {
			// Unlink the temporary files.
			unlink($keyfile);
			unlink($crtfile);
			throw new \OMV\ExecException($cmd->getCommandLine(), $output);
		}
		// Read certificate and key content, then unlink the temporary files.
		$keydata = file_get_contents($keyfile);
		$crtdata = file_get_contents($crtfile);
		unlink($keyfile);
		unlink($crtfile);
		// Finally import generated certificate.
		return $this->callMethod("set", array(
			  "uuid" => $GLOBALS['OMV_UUID_UNDEFINED'],
			  "certificate" => $crtdata,
			  "privatekey" => $keydata,
			  "comment" => !empty($subject) ? $subject : ""
		  ), $context);
	}

	/**
	 * Get list of SSH certificate configuration objects.
	 * @param params An array containing the following fields:
	 *   \em start The index where to start.
	 *   \em limit The number of objects to process.
	 *   \em sortfield The name of the column used to sort.
	 *   \em sortdir The sort direction, ASC or DESC.
	 * @param context The context of the caller.
	 * @return An array containing the requested objects. The field \em total
	 *   contains the total number of objects, \em data contains the object
	 *   array. An exception will be thrown in case of an error.
	 */
	public function getSshList($params, $context) {
		// Validate the RPC caller context.
		$this->validateMethodContext($context, array(
			  "role" => OMV_ROLE_ADMINISTRATOR
		  ));
		// Validate the parameters of the RPC service method.
		$this->validateMethodParams($params, "rpc.common.getlist");
		// Get list of SSH certificate configuration objects.
		$db = \OMV\Config\Database::getInstance();
		$objects = $db->get("conf.system.certificate.ssh");
		// Process the list of objects.
		$result = [];
		foreach ($objects as $objectk => $objectv) {
			// Add additional properties.
			$objectv->add("_used", "boolean");
			// Remove the private key. It should not leave the system.
			$objectv->remove("privatekey");
			// Set '_used' flag if the certificate is referenced.
			$objectv->set("_used", $db->isReferenced($objectv));
			// Get the values.
			$result[] = $objectv->getAssoc();
		}
		// Filter the result.
		return $this->applyFilter($result, $params['start'], $params['limit'],
		  $params['sortfield'], $params['sortdir']);
	}

	/**
	 * Create a SSH certificate.
	 * @param params An array containing the following fields:
	 *   \em comment The comment
	 * @param context The context of the caller.
	 * @return The configuration object.
	 */
	public function createSsh($params, $context) {
		// Validate the RPC caller context.
		$this->validateMethodContext($context, array(
			  "role" => OMV_ROLE_ADMINISTRATOR
		  ));
		// Validate the parameters of the RPC service method.
		$this->validateMethodParams($params, "rpc.certificatemgmt.createssh");
		// Create the requested certificate.
		$file = tempnam(sys_get_temp_dir(), "id_rsa");
		$cmdArgs = [];
		$cmdArgs[] = "-t rsa";
		$cmdArgs[] = "-P ''";
		$cmdArgs[] = sprintf("-C %s", escapeshellarg(trim(array_value(
		  $params, "comment", ""))));
		$cmdArgs[] = sprintf("-f %s", escapeshellarg($file));
		$cmd = sprintf("export LANG=C; (echo y) | ssh-keygen %s 2>&1",
		  implode(" ", $cmdArgs));
		if (0 !== $this->exec($cmd, $output)) {
			// Unlink the temporary files.
			unlink($file);
			unlink($file.".pub");
			throw new \OMV\ExecException($cmd, $output);
		}
		// Read certificate and key content, then unlink the temporary files.
		$privatekey = file_get_contents($file);
		$publickey = file_get_contents($file.".pub");
		unlink($file);
		unlink($file.".pub");
		// Finally import generated certificate.
		return $this->callMethod("setSsh", array(
			  "uuid" => $GLOBALS['OMV_UUID_UNDEFINED'],
			  "publickey" => $publickey,
			  "privatekey" => $privatekey,
			  "comment" => array_value($params, "comment", "")
		  ), $context);
	}

	/**
	 * Get a SSH certificate configuration object.
	 * @param params An array containing the following fields:
	 *   \em uuid The UUID of the configuration object.
	 * @param context The context of the caller.
	 * @return The requested configuration object.
	 */
	function getSsh($params, $context) {
		// Validate the RPC caller context.
		$this->validateMethodContext($context, array(
			  "role" => OMV_ROLE_ADMINISTRATOR
		  ));
		// Validate the parameters of the RPC service method.
		$this->validateMethodParams($params, "rpc.common.objectuuid");
		// Get the configuration object.
		$db = \OMV\Config\Database::getInstance();
		$object = $db->get("conf.system.certificate.ssh", $params['uuid']);
		// Remove the private key. It should not leave the system.
		$object->remove("privatekey");
		// Return the values.
		return $object->getAssoc();
	}

	/**
	 * Set (add/update) a SSH certificate configuration object
	 * @param params The method parameters.
	 * @param context The context of the caller.
	 * @return The stored configuration object.
	 */
	function setSsh($params, $context) {
		// Validate the RPC caller context.
		$this->validateMethodContext($context, array(
			  "role" => OMV_ROLE_ADMINISTRATOR
		  ));
		// Validate the parameters of the RPC service method.
		$this->validateMethodParams($params, "rpc.certificatemgmt.setssh");
		// Prepare the configuration object.
		$object = new \OMV\Config\ConfigObject("conf.system.certificate.ssh");
		$object->setAssoc($params);
		// Validate the private key.
		if (!$object->isEmpty("privatekey")) {
			if (!($privatekey = openssl_pkey_get_private(
			  $object->get("privatekey")))) {
				throw new \OMV\Exception("Invalid private key: %s",
				  openssl_error_string());
			}
			openssl_pkey_free($privatekey);
		}
		// Set the configuration data.
		$isNew = $object->isNew();
		if (TRUE === $isNew) {
			// The private key is required here.
			if ($object->isEmpty("privatekey"))
				throw new \OMV\Exception("Private key does not exist.");
		}
		$db = \OMV\Config\Database::getInstance();
		$db->set($object);
		// Notify configuration changes.
		$dispatcher = \OMV\Engine\Notify\Dispatcher::getInstance();
		$dispatcher->notify($isNew ? OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
		  "org.openmediavault.conf.system.certificate.ssh",
		  $object->getAssoc());
	}

	/**
	 * Delete a SSH certificate config object.
	 * @param params An array containing the following fields:
	 *   \em uuid The UUID of the configuration object.
	 * @param context The context of the caller.
	 * @return The deleted configuration object.
	 */
	public function deleteSsh($params, $context) {
		// Validate the RPC caller context.
		$this->validateMethodContext($context, array(
			  "role" => OMV_ROLE_ADMINISTRATOR
		  ));
		// Validate the parameters of the RPC service method.
		$this->validateMethodParams($params, "rpc.common.objectuuid");
		// Check if the certificate is referenced.
		$db = \OMV\Config\Database::getInstance();
		$object = $db->get("conf.system.certificate.ssh", $params['uuid']);
		$db->assertIsNotReferenced($object);
		// Delete the configuration object.
		return $this->deleteConfigObjectByObject($object,
		  "org.openmediavault.conf.system.certificate.ssh");
	}
}
