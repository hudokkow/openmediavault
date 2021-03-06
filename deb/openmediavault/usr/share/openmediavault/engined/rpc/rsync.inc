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
require_once("openmediavault/functions.inc");

class OMVRpcServiceRsync extends \OMV\Rpc\ServiceAbstract {
	/**
	 * Get the RPC service name.
	 */
	public function getName() {
		return "Rsync";
	}

	/**
	 * Initialize the RPC service.
	 */
	public function initialize() {
		$this->registerMethod("getList");
		$this->registerMethod("get");
		$this->registerMethod("set");
		$this->registerMethod("delete");
		$this->registerMethod("execute");
	}

	/**
	 * Get list of rsync job configuration objects.
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
		// Get configuration objects.
		$db = \OMV\Config\Database::getInstance();
		$objects = $db->get("conf.service.rsync.job");
		// Add additional information.
		$objectsAssoc = [];
		foreach ($objects as $objectk => &$objectv) {
			$objectv->set("srcname", gettext("n/a"));
			$objectv->set("destname", gettext("n/a"));
			switch ($objectv->get("type")) {
			case "local":
				// Get the shared folder configuration object and update the
				// 'srcname' property.
				$sfObject = $db->get("conf.system.sharedfolder",
				  $objectv->get("src"));
				$objectv->set("srcname", $sfObject->get("name"));
				// Get the shared folder configuration object and update the
				// 'destname' property.
				$sfObject = $db->get("conf.system.sharedfolder",
				  $objectv->get("dest"));
				$objectv->set("destname", $sfObject->get("name"));
				break;
			case "remote":
				switch($objectv['mode']) {
				case "push":
					$sfObject = $db->get("conf.system.sharedfolder",
					  $objectv->get("src"));
					$objectv->set("srcname", $sfObject->get("name"));
					$objectv->copy("dest", "destname");
					break;
				case "pull":
					$sfObject = $db->get("conf.system.sharedfolder",
					  $objectv->get("dest"));
					$objectv->set("destname", $sfObject->get("name"));
					$objectv->copy("src", "srcname");
					break;
				}
				break;
			}
			$objectsAssoc[] = $objectv->getAssoc();
		}
		// Filter the result.
		return $this->applyFilter($objectsAssoc, $params['start'],
		  $params['limit'], $params['sortfield'], $params['sortdir']);
	}

	/**
	 * Get a rsync job configuration object.
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
		$object = $db->get("conf.service.rsync.job", $params['uuid']);
		// Add additional information.
		switch ($object->get("type")) {
		case "local":
			$object->copy("src", "srcsharedfolderref");
			$object->copy("dest", "destsharedfolderref");
		case "remote":
			switch($object->get("mode")) {
			case "push":
				$object->copy("src", "srcsharedfolderref");
				$object->copy("dest", "desturi");
				break;
			case "pull":
				$object->copy("src", "srcuri");
				$object->copy("dest", "destsharedfolderref");
				break;
			}
			break;
		}
		return $object->getAssoc();
	}

	/**
	 * Set a rsync job configuration object.
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
		$this->validateMethodParams($params, "rpc.rsync.set");
		// Prepare the configuration data.
		$object = new \OMV\Config\ConfigObject("conf.service.rsync.job");
		$object->setAssoc($params);
		// Set the configuration object.
		$isNew = $object->isNew();
		$db = \OMV\Config\Database::getInstance();
		$db->set($object);
		// Notify configuration changes.
		$dispatcher = \OMV\Engine\Notify\Dispatcher::getInstance();
		$dispatcher->notify($isNew ? OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
		  "org.openmediavault.conf.service.rsync.job", $object->getAssoc());
		// Return the configuration object.
		return $object->getAssoc();
	}

	/**
	 * Delete a rsync job configuration object.
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
		// Delete the configuration object.
		return $this->deleteConfigObjectByUuid("conf.service.rsync.job",
		  $params['uuid'], "org.openmediavault.conf.service.rsync.job");
	}

	/**
	 * Execute a rsync job.
	 * @param params An array containing the following fields:
	 *   \em uuid The UUID of the cron job to execute.
	 * @param context The context of the caller.
	 * @return The name of the background process status file.
	 * @throw \OMV\Config\ConfigDirtyException
	 */
	public function execute($params, $context) {
		// Validate the RPC caller context.
		$this->validateMethodContext($context, array(
			  "role" => OMV_ROLE_ADMINISTRATOR
		  ));
		// Validate the parameters of the RPC service method.
		$this->validateMethodParams($params, "rpc.common.objectuuid");
		// Check if the module is marked as dirty. This is an indication
		// that the rsync cron script has not been created or updated
		// until now.
		if ($this->isModuleDirty("rsync"))
			throw new \OMV\Config\ConfigDirtyException();
		// Create a background process.
		$bgStatusFilename = $this->createBgProcStatus();
		$pid = $this->fork();
		if ($pid > 0) { // Parent process.
			$this->initializeBgProcStatus($bgStatusFilename, $pid);
			return $bgStatusFilename;
		}
		// Child process.
		try {
			$bgOutputFilename = $this->createBgProcOutput();
			$this->updateBgProcStatus($bgStatusFilename, "outputfilename",
			  $bgOutputFilename);
			// Execute the rsync cron script.
			$cmd = "/bin/sh";
			$cmdArgs = [];
			$cmdArgs[] = build_path(DIRECTORY_SEPARATOR,
			  $GLOBALS['OMV_CRONSCRIPTS_DIR'],
			  sprintf("rsync-%s", $params['uuid']));
			if (FALSE === $this->execve($cmd, $cmdArgs, $bgOutputFilename))
				throw new \OMV\ExecException($cmd." ".implode(" ", $cmdArgs));
		} catch(\Exception $e) {
			$this->finalizeBgProcStatus($bgStatusFilename, "", $e);
			exit(1);
		}
	}
}
