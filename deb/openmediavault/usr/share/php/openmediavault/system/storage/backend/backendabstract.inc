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
namespace OMV\System\Storage\Backend;

/**
 * The generic class that represents a storage device backend.
 * @ingroup api
 *
 * To implement a new storage backend the following information are
 * necessary:
 * # cat /proc/partitions
 * # export LANG=C; blkid
 * # export LANG=C; udevadm info --query=property --name=/dev/<DEVNAME>
 * # cat /sys/block/<DEVNAME>/device/model
 * # cat /sys/block/<DEVNAME>/device/vendor
 */
abstract class BackendAbstract {
	/**
	 * Get the type of the storage device backend, e.g.
	 * OMV_STORAGE_DEVICE_TYPE_HDD, OMV_STORAGE_DEVICE_TYPE_HBA, ...
	 */
	abstract function getType();

	/**
	 * Get a list of devices of the given storage device backend.
	 * @return A list of devicefile names, otherwise FALSE.
	 */
	abstract function enumerate();

	/**
	 * Get a list of all devices that are used by devices of this
	 * storage device backend.
	 * @return A list of devicefile names, otherwise FALSE.
	 */
	public function enumerateSlaves() {
		return array();
	}

	/**
	 * Check whether the given device is represented by this storage
	 * device backend.
	 * @param deviceFile Specifies the device file, e.g.
	 *   <ul>
	 *   \li /dev/sdb
	 *   \li /dev/md1
	 *   \li /dev/cciss/c0d0
	 *   \li /dev/disk/by-id/scsi-SATA_ST3200XXXX2AS_5XWXXXR6
	 *   \li /dev/disk/by-label/DATA
	 *   \li /dev/disk/by-path/pci-0000:00:10.0-scsi-0:0:0:0
	 *   \li /dev/disk/by-uuid/ad3ee177-777c-4ad3-8353-9562f85c0895
	 *   </ul>
	 * @return TRUE if successful, otherwise FALSE.
	 */
	public function isTypeOf($deviceFile) {
		return FALSE;
	}

	/**
	 * Get the object of the class that represents and implements a device
	 * of this storage device backend.
	 * @param args The arguments to the class constructor.
	 * @return The object of the class implementing the given device type,
	 *   otherwise NULL.
	 */
	public function getImpl($args) {
		return NULL;
	}

	/**
	 * Returns base device file by stripping the partition appendix.
	 * @param deviceFile Specifies the device file, e.g.
	 *   <ul>
	 *   \li /dev/sdb1 => /dev/sdb
	 *   \li /dev/cciss/c0d0p2 => /dev/cciss/c0d0
	 *   \li /dev/mapper/vg0-lv0 => /dev/mapper/vg0-lv0
	 *   \li /dev/dm-0 => /dev/dm-0
	 *   \li /dev/md0 => /dev/md0
	 *   \li /dev/loop0 => /dev/loop0
	 *   </ul>
	 * @return The base device file.
	 */
	public function baseDeviceFile($deviceFile) {
		return $deviceFile;
	}

	/**
	 * Return the device file to be used to create a filesystem.
	 * @note Only one partition per storage device is supported.
	 * @param deviceFile Specifies the device file, e.g.
	 *   <ul>
	 *   \li /dev/sdb => /dev/sdb1
	 *   \li /dev/cciss/c0d0 => /dev/cciss/c0d0p1
	 *   \li /dev/mapper/vg0-lv0 => /dev/mapper/vg0-lv0
	 *   \li /dev/dm-0 => /dev/dm-0
	 *   \li /dev/md0 => /dev/md0
	 *   </ul>
	 * @return The device file. Defaults to /dev/xxx1.
	 */
	public function fsDeviceFile($deviceFile) {
		return sprintf("%s1", $deviceFile);
	}

	/**
	 * Helper function to enumerate the devices represented by this storage
	 * device backend via the /proc filesystem.
	 * @param regex The regular expression used to identify the devices
	 *   represented by this storage backend.
	 * @return A list of devicefile names, otherwise FALSE.
	 */
	final protected function enumerateProcFs($regex) {
		$result = [];
		$regex = sprintf('/^\s+(\d+)\s+(\d+)\s+(\d+)\s+(%s)$/', $regex);
		foreach(file("/proc/partitions") as $outputk => $outputv) {
			if (1 !== preg_match($regex, $outputv, $matches))
				continue;
			$result[] = sprintf("/dev/%s", $matches[4]);
		}
		return $result;
	}

	/**
	 * Helper function to check whether the given device is represented by
	 * this storage device backend.
	 * @param deviceFile Specifies the device file.
	 * @param regex The regular expression used to identify the devices
	 *   represented by this storage backend.
	 */
	final protected function isTypeOfByName($deviceFile, $regex) {
		// If the device file looks like /dev/disk/by-(id|label|path|uuid)/*
		// then it is necessary to get the /dev/xxx equivalent.
		if (1 == preg_match("/^\/dev\/disk\/by-.+$/", $deviceFile))
			$deviceFile = realpath($deviceFile);
		// Extract the device name, e.g.:
		// - /dev/sdb => sdb
		// - /dev/cciss/c0d0 => cciss/c0d0
		// - /dev/mapper/vg0-lv0 => mapper/vg0-lv0
		// - /dev/loop1 => loop1
		$deviceName = str_replace("/dev/", "", $deviceFile);
		// Check if the device name matches the given regular expression.
		return (1 == preg_match("/^{$regex}$/i", $deviceName));
	}
}
