from onl.platform.base import *
from onl.platform.delta import *

class OnlPlatform_x86_64_delta_ag9032v2_r0(OnlPlatformDelta,
                                              OnlPlatformPortConfig_32x100):
    PLATFORM='x86-64-delta-ag9032v2-r0'
    MODEL="AG9032V2"
    SYS_OBJECT_ID=".9032.2"


    def baseconfig(self):
        # kernel module i2c_ismt realpath
        # /lib/modules/4.9.75-OpenNetworkLinux/kernel/drivers/i2c/busses
        ret = False

        findcmd = "find /lib/modules/%s/kernel -iname 'i2c*ismt.ko'" % os.uname()[2]
        module_p = subprocess.check_output(findcmd, shell=True).strip()

        if (os.path.exists(module_p)):
            subprocess.check_call("insmod %s" % module_p, shell=True);
            ret = True
        else:
            msg("Kernel module could not be found.\n")
            ret = False

        if ret:
            # self.insmod('i2c_ismt')
            cmd = "echo 'blacklist delta_i2c_ismt' > /etc/modprobe.d/blacklist.conf"
            subprocess.check_call(cmd, shell=True);

        #IDEEPROM modulize
        os.system("modprobe at24")
        self.new_i2c_device('24c02', 0x53, 1)

        return True


