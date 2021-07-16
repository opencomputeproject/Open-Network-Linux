from onl.platform.base import *
from onl.platform.inventec import *
import logging

class OnlPlatform_x86_64_inventec_d7032q28b_r0(OnlPlatformInventec,
                                              OnlPlatformPortConfig_32x100):
    PLATFORM='x86-64-inventec-d7032q28b-r0'
    MODEL="X86-D7032Q28B"
    SYS_OBJECT_ID=".7032.1"

    _path_prefix_list=[
        "/sys/devices/platform/coretemp.0/hwmon/",
    ]
    _path_dst_list=[
        "/var/coretemp",
    ]

    def baseconfig(self):
        os.system("insmod /lib/modules/`uname -r`/kernel/drivers/gpio/gpio-ich.ko gpiobase=0")
        self.insmod('inv_platform')
        self.insmod('inv_psoc')
        self.insmod('inv_cpld')
        self.new_i2c_device('inv_eeprom', 0x53, 0)
        self.insmod('inv_eeprom')
        self.insmod('swps')
        #self.insmod('vpd')
        self.insmod('inv_pthread')

        for i in range(0,len(self._path_prefix_list)):
            if( os.path.islink(self._path_dst_list[i]) ):
                os.unlink(self._path_dst_list[i])
                logging.warning("Path %s exists, remove before link again" % self._path_dst_list[i] )
            self.link_dir(self._path_prefix_list[i],self._path_dst_list[i])

        return True

    def link_dir(self,prefix,dst):
        ret=os.path.isdir(prefix)
        if ret==True:
            dirs=os.listdir(prefix)
            ret=False
            for i in range(0,len(dirs)):
                if 'hwmon' in dirs[i]:
                    src=prefix+dirs[i]
                    os.symlink(src,dst)
                    ret=True
                    break
            if ret==False:
                logging.warning("Can't find proper dir to link under %s" % prefix)            
        else:
            logging.warning("Path %s is not a dir" % prefix)