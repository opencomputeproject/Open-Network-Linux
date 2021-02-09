from onl.platform.base import *
from onl.platform.netberg import *

class OnlPlatform_x86_64_netberg_aurora_610_r0(OnlPlatformNetberg,
                                              OnlPlatformPortConfig_48x25_8x100):
    PLATFORM='x86-64-netberg-aurora-610-r0'
    MODEL="AURORA610"
    SYS_OBJECT_ID=".610.1"

    def baseconfig(self):
        os.system("insmod /lib/modules/`uname -r`/kernel/drivers/gpio/gpio-ich.ko gpiobase=0")
        os.system("insmod /lib/modules/`uname -r`/kernel/drivers/mfd/lpc_ich.ko")
        self.insmod('i2c-gpio')
        self.insmod('net_platform')
        self.insmod('net_psoc')
        os.system("echo net_cpld 0x77 > /sys/bus/i2c/devices/i2c-0/new_device")
        self.insmod('net_cpld')
        self.insmod('swps')
        self.insmod('vpd')
        os.system("/lib/platform-config/x86-64-netberg-aurora-610-r0/onl/healthstatus.sh &")

        return True

