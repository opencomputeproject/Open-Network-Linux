from onl.platform.base import *
from onl.platform.accton import *

class OnlPlatform_x86_64_accton_as9516_32d_r0(OnlPlatformAccton,
                                                OnlPlatformPortConfig_32x400):
    MODEL="as-9516-32d"
    PLATFORM="x86-64-accton-as9516-32d-r0"
    SYS_OBJECT_ID=".9516.32.1"

    def baseconfig(self):
        self.insmod('optoe')
                       
        subprocess.call('ifconfig usb0 up', shell=True)

	self.insmod("bf_fpga.ko")


        return True
