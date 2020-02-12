from onl.platform.base import *
from onl.platform.inventec import *

class OnlPlatform_x86_64_inventec_d10064_r0(OnlPlatformInventec,
                                              OnlPlatformPortConfig_64x100):
    PLATFORM='x86-64-inventec-d10064-r0'
    MODEL="D10064"
    SYS_OBJECT_ID=".1.32"

    def baseconfig(self):
        os.system("insmod /lib/modules/`uname -r`/kernel/drivers/gpio/gpio-ich.ko gpiobase=0")

        #self.insmod('inv_platform')
        os.system("echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-0/new_device")
        os.system("sleep 1")
        os.system("echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-8/new_device")
        os.system("sleep 1")
        #Upper
        os.system("echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-1/new_device")
        os.system("echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-2/new_device")
        os.system("echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-3/new_device")
        os.system("echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-4/new_device")
        os.system("sleep 1")
        #Lower
        os.system("echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-9/new_device")
        os.system("echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-10/new_device")
        os.system("echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-11/new_device")
        os.system("echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-12/new_device")

        self.insmod('inv_psoc')
        os.system("echo inv_cpld 0x55 > /sys/bus/i2c/devices/i2c-0/new_device")
        os.system("echo inv_cpld 0x77 > /sys/bus/i2c/devices/i2c-0/new_device")
        self.insmod('inv_cpld')
        self.insmod('swps')
        self.insmod('vpd')

        os.system("/lib/platform-config/x86-64-inventec-d10064-r0/onl/healthstatus.sh &")

        return True
