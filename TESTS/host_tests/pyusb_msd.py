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


class PyusbMSDTest(BaseHostTest):
    """Host side test for USB MSD class."""

    __result = None
    MOUNT_WAIT_TIME = 25 # in [s]
    initial_disk_list = None
    msd_disk = None
    
    def _callback_record_disk_list(self, key, value, timestamp):
        """Records msd disks list.

        It will be used to find new disks mounted during test.
        """
        self.initial_disk_list = set(MSDUtils.disks())
        self.report_success()

    def _callback_check_file_exist(self, key, value, timestamp):
        """Check if file exist.

        """
        folder_name, file_name, file_content = value.split(' ')
        file_path = os.path.join(self.msd_disk.mountpoint, folder_name, file_name)
        try:
            file = open(file_path, 'r')
            line = file.readline()
            file.close()
            time.sleep(2) # wait for msd communication done
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
        dir_name, file_name = value.split(' ')

        try:
            os.remove(os.path.join(self.msd_disk.mountpoint, dir_name, file_name))
        except:
            self.report_error("delete files")
            return
        time.sleep(2) # wait for msd communication done
        self.report_success()

    def _callback_check_if_mounted(self, key, value, timestamp):
        """Check if disk was mounted.

        """
        wait_time = self.MOUNT_WAIT_TIME
        while wait_time != 0:
            disk_list = set(MSDUtils.disks()) - self.initial_disk_list
            if len(disk_list) == 1: # one new MSD disk found
                self.msd_disk = disk_list.pop()
                time.sleep(2) # wait for msd communication done
                self.report_success()
                return
            elif len(disk_list) > 1:
                self.log('detected more then one new USB disk!!!!!!: {}'.format(disk_list))
            wait_time -= 1
            time.sleep(1) # wait 1s and try again
        self.report_error("mount check")

    def _callback_check_if_not_mounted(self, key, value, timestamp):
        """Check if disk was unmouted.

        """
        wait_time = self.MOUNT_WAIT_TIME
        while wait_time != 0:
            disk_list = set(MSDUtils.disks()) - self.initial_disk_list
            if len(disk_list) == 0:
                self.msd_disk = None
                time.sleep(2) # wait for msd communication done
                self.report_success()
                return
            wait_time -= 1
            time.sleep(1) # wait 1s and try again
        self.report_error("unmount check")

    def _callback_get_mounted_fs_size(self, key, value, timestamp):
        """Record visible filesystem size.

        """
        stats = psutil.disk_usage(self.msd_disk.mountpoint)
        self.send_kv("{}".format(stats.total), "0")

    def _callback_unmount(self, key, value, timestamp):
        """Disk unmount.

        """
        if MSDUtils.unmount(self.msd_disk):
            self.report_success()
        else:
            self.report_error("unmount")

    def setup(self):
        self.register_callback('record_disk_list', self._callback_record_disk_list)
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
    @staticmethod
    def disks():
        system_name = platform.system()
        if system_name == "Windows":
            return MSDUtils._disks_windows()
        elif system_name == "Linux":
            return MSDUtils._disks_linux()
        elif system_name == "Darwin":
            return MSDUtils._disks_mac()
        return []

    @staticmethod
    def unmount(disk):
        system_name = platform.system()
        if system_name == "Windows":
            return MSDUtils._unmount_windows(disk)
        elif system_name == "Linux":
            return MSDUtils._unmount_linux(disk)
        elif system_name == "Darwin":
            return MSDUtils._unmount_mac(disk)
        return False

    @staticmethod
    def _unmount_windows(disk):

        tmp_file = tempfile.NamedTemporaryFile(suffix='.ps1', delete=False)
        try:
            # create unmount script
            tmp_file.write('$disk_leter=$args[0]\n')
            tmp_file.write('$driveEject = New-Object -comObject Shell.Application\n')
            tmp_file.write('$driveEject.Namespace(17).ParseName($disk_leter).InvokeVerb("Eject")\n')
            # close to allow open by other process
            tmp_file.close()

            try_cout = 10
            while try_cout:
                p = subprocess.Popen(["powershell.exe", tmp_file.name + " " + disk.mountpoint], stdout=sys.stdout)
                p.communicate()
                try_cout -= 1
                disks = set(MSDUtils._disks_windows())
                if disk not in disks:
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

    @staticmethod
    def _disks_windows():
        msd_disks = []
        disks = psutil.disk_partitions()
        for disk in disks:
            opts = disk.opts.split(",")
            for opt in opts:
                if opt == "removable":
                    msd_disks.append(disk)
        return msd_disks

    @staticmethod
    def _disks_linux():
        msd_disks = []
        disks = psutil.disk_partitions()
        for disk in disks:
            if "/dev/sd" in disk.device and "/media/" in disk.mountpoint:
                msd_disks.append(disk)
        return msd_disks

    @staticmethod
    def _disks_mac():
        msd_disks = []
        disks = psutil.disk_partitions()
        for disk in disks:
            if "/dev/disk" in disk.device and "/Volumes/" in disk.mountpoint:
                msd_disks.append(disk)
        return msd_disks
