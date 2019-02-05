"""
Copyright (c) 2019, Arm Limited and affiliates.
SPDX-License-Identifier: Apache-2.0

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

from mbed_host_tests import BaseHostTest
import time
import psutil
import tempfile
import os
import platform
import subprocess
import sys
system_name = platform.system()
if system_name == "Windows":
    import wmi


class PyusbMSDTest(BaseHostTest):
    """Host side test for USB MSD class."""

    __result = None
    MOUNT_WAIT_TIME = 25  # in [s]
    initial_disk_list = None
    msd_disk = None

    def _callback_check_file_exist(self, key, value, timestamp):
        """Check if file exist.

        """
        folder_name, file_name, file_content, serial_number = value.split(' ')
        msd_disk = MSDUtils.disk_path(serial_number)
        file_path = os.path.join(msd_disk, folder_name, file_name)
        try:
            file = open(file_path, 'r')
            line = file.readline()
            file.close()
            time.sleep(2)  # wait for msd communication done
            if line == file_content:
                self.send_kv("exist", "0")
                return
            self.report_error("file content invalid")
        except IOError as err:
            self.log('{} !!!'.format(err))
        self.send_kv("non-exist", "0")

    def _callback_delete_files(self, key, value, timestamp):
        """Delete test file.

        """
        dir_name, file_name, serial_number = value.split(' ')
        msd_disk = MSDUtils.disk_path(serial_number)
        try:
            os.remove(os.path.join(msd_disk, dir_name, file_name))
        except:
            self.report_error("delete files")
            return
        time.sleep(2)  # wait for msd communication done
        self.report_success()

    def _callback_check_if_mounted(self, key, value, timestamp):
        """Check if disk was mounted.

        """
        serial_number = value
        wait_time = self.MOUNT_WAIT_TIME
        while wait_time != 0:
            msd_disk = MSDUtils.disk_path(serial_number)
            if msd_disk is not None:
                # MSD disk found
                time.sleep(2)  # wait for msd communication done
                self.report_success()
                return
            wait_time -= 1
            time.sleep(1)  # wait 1s and try again
        self.report_error("mount check")

    def _callback_check_if_not_mounted(self, key, value, timestamp):
        """Check if disk was unmouted.

        """
        serial_number = value
        wait_time = self.MOUNT_WAIT_TIME
        while wait_time != 0:
            msd_disk = MSDUtils.disk_path(serial_number)
            if msd_disk is None:
                #self.msd_disk = None
                time.sleep(2)  # wait for msd communication done
                self.report_success()
                return
            wait_time -= 1
            time.sleep(1)  # wait 1s and try again
        self.report_error("unmount check")

    def _callback_get_mounted_fs_size(self, key, value, timestamp):
        """Record visible filesystem size.

        """
        serial_number = value
        stats = psutil.disk_usage(MSDUtils.disk_path(serial_number))
        self.send_kv("{}".format(stats.total), "0")

    def _callback_unmount(self, key, value, timestamp):
        """Disk unmount.

        """
        serial_number = value
        if MSDUtils.unmount(serial=serial_number):
            self.report_success()
        else:
            self.report_error("unmount")

    def setup(self):
        self.register_callback('check_if_mounted', self._callback_check_if_mounted)
        self.register_callback('check_if_not_mounted', self._callback_check_if_not_mounted)
        self.register_callback('get_mounted_fs_size', self._callback_get_mounted_fs_size)
        self.register_callback('check_file_exist', self._callback_check_file_exist)
        self.register_callback('delete_files', self._callback_delete_files)
        self.register_callback('unmount', self._callback_unmount)

    def report_success(self):
        self.send_kv("passed", "0")

    def report_error(self, msg):
        self.log('{} failed !!!'.format(msg))
        self.send_kv("failed", "0")

    def result(self):
        return self.__result

    def teardown(self):
        pass


class MSDUtils(object):
    # @staticmethod
    # def disks():
    #     system_name = platform.system()
    #     if system_name == "Windows":
    #         return MSDUtils._disks_windows()
    #     elif system_name == "Linux":
    #         return MSDUtils._disks_linux()
    #     elif system_name == "Darwin":
    #         return MSDUtils._disks_mac()
    #     return []

    @staticmethod
    def disk_path(serial):
        system_name = platform.system()
        if system_name == "Windows":
            return MSDUtils._disk_path_windows(serial)
        elif system_name == "Linux":
            return MSDUtils._disk_path_linux(serial)
        elif system_name == "Darwin":
            return MSDUtils._disk_path_mac(serial)
        return None

    @staticmethod
    def unmount(serial):
        system_name = platform.system()
        if system_name == "Windows":
            return MSDUtils._unmount_windows(serial)
        elif system_name == "Linux":
            return MSDUtils._unmount_linux(serial)
        elif system_name == "Darwin":
            return MSDUtils._unmount_mac(serial)
        return False


    @staticmethod
    def _disk_path_windows(serial):
        #print "#########MSDUtils.disk(sreial: {})".format(serial)
        serial_decoded = serial.encode("ascii")
        #print "#########MSDUtils.disk(serial_decoded: {})".format(serial_decoded)
        c = wmi.WMI()
        for physical_disk in c.Win32_DiskDrive():
            #print "#########MSDUtils.disk( physical_disk.SerialNumber {})".format(physical_disk.SerialNumber)
            #serial_decoded = physical_disk.SerialNumber.encode("ascii")
            #print "#########MSDUtils.disk( serial_decoded {})".format(serial_decoded)
            if serial_decoded == physical_disk.SerialNumber:
                for partition in physical_disk.associators("Win32_DiskDriveToDiskPartition"):
                    for logical_disk in partition.associators("Win32_LogicalDiskToPartition"):
                        #print physical_disk.Caption, partition.Caption, logical_disk.Caption
                        #print "physical_disk\n {}".format(physical_disk)
                        #print "partition\n {}".format(partition)
                        #print "logical_disk\n {}".format(logical_disk)
                        return logical_disk.Caption
        return None

    @staticmethod
    def _disk_path_linux(serial):
        return None

    @staticmethod
    def _disk_path_mac(serial):
        return None

    @staticmethod
    def _unmount_windows(serial):
        disk_path = MSDUtils.disk_path(serial)
        tmp_file = tempfile.NamedTemporaryFile(suffix='.ps1', delete=False)
        try:
            # create unmount script
            tmp_file.write('$disk_leter=$args[0]\n')
            tmp_file.write('$driveEject = New-Object -comObject Shell.Application\n')
            tmp_file.write('$driveEject.Namespace(17).ParseName($disk_leter).InvokeVerb("Eject")\n')
            # close to allow open by other process
            tmp_file.close()

            try_count = 10
            while try_count:
                p = subprocess.Popen(["powershell.exe", tmp_file.name + " " + disk_path], stdout=sys.stdout)
                p.communicate()
                try_count -= 1
                if MSDUtils._disk_path_windows(serial) is None:
                    return True
                time.sleep(1)
        finally:
            os.remove(tmp_file.name)

        return False

    @staticmethod
    def _unmount_linux(disk):
        os.system("umount " + disk.mountpoint)
        disks = set(MSDUtils._disks_linux())
        if disk not in disks:
            return True
        return False

    @staticmethod
    def _unmount_mac(disk):
        os.system("diskutil unmount " + disk.mountpoint)
        disks = set(MSDUtils._disks_mac())
        if disk not in disks:
            return True
        return False
