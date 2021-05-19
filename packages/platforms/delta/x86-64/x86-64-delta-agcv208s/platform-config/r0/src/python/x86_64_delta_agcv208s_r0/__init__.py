from onl.platform.base import *
from onl.platform.delta import *

class OnlPlatform_x86_64_delta_agcv208s_r0(OnlPlatformDelta,
                                              OnlPlatformPortConfig_4x10_8x25_2x100):
    PLATFORM='x86-64-delta-agcv208s-r0'
    MODEL="AGCV208S"
    SYS_OBJECT_ID=".208.1"


    def baseconfig(self):
        os.system("echo 'remove,bt,i/o,0xE4' > /sys/module/ipmi_si/parameters/hotmod")
        os.system("echo 'add,kcs,i/o,0xca2' > /sys/module/ipmi_si/parameters/hotmod")

        return True


