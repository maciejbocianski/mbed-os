# USB mass storage test user guide

To run the tests-usb_device-msd test you need device with:
- at least 70kB of RAM 
- and/or with flash block device support

Test run command (for GCC_ARM and k64f target):
```bash
mbed test -t GCC_ARM -m k64f -n tests-usb_device-msd --app-config ./TESTS/usb_device/msd/mbed_app.json
```
