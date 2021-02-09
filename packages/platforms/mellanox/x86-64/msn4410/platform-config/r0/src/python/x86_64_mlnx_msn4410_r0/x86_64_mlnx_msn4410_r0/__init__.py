from onl.platform.base import *
from onl.platform.mellanox import *

class OnlPlatform_x86_64_mlnx_msn4410_r0(OnlPlatformMellanox,
                                        OnlPlatformPortConfig_32x100):
    PLATFORM='x86-64-mlnx-msn4410-r0'
    MODEL="MSN4410"
    SYS_OBJECT_ID=".4700.2"

    def baseconfig(self):
        # load modules
        import os
        # necessary if there are issues with the install
        # os.system("/usr/bin/apt-get install")
        self.syseeprom_export();
        return True
