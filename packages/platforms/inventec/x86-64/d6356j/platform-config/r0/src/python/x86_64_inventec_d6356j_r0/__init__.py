from onl.platform.base import *
from onl.platform.inventec import *
import logging

class OnlPlatform_x86_64_inventec_d6356j_r0(OnlPlatformInventec,
                                            OnlPlatformPortConfig_48x25_8x100):
    PLATFORM='x86-64-inventec-d6356j-r0'
    MODEL="D6356J"
    SYS_OBJECT_ID=".6356j.1"

    _path_prefix_list=[
        "/sys/bus/i2c/devices/4-005b/hwmon/",
        "/sys/bus/i2c/devices/4-005a/hwmon/",
        "/sys/devices/platform/coretemp.0/hwmon/",
        "/sys/bus/i2c/devices/3-0048/hwmon/",
        "/sys/bus/i2c/devices/3-004a/hwmon/",
        "/sys/bus/i2c/devices/3-004d/hwmon/",
        "/sys/bus/i2c/devices/3-004e/hwmon/"
    ]
    _path_dst_list=[
        "/var/psu1",
        "/var/psu2",
        "/var/coretemp",
        "/var/thermal_6",
        "/var/thermal_7",        
        "/var/thermal_8",
        "/var/thermal_9",
    ]
    def baseconfig(self):
        os.system("rmmod gpio_ich")
        os.system("insmod /lib/modules/`uname -r`/kernel/drivers/gpio/gpio-ich.ko gpiobase=0")
        self.insmod('i2c-gpio')
        self.insmod('pmbus_core')
        self.insmod('pmbus')
        self.insmod('ucd9000')
        self.insmod('inv-i2c-mux-pca9641')
        self.insmod('inv_platform')
        self.insmod('inv_cpld')
        self.insmod('swps')
        self.insmod('inv_eeprom')
        self.new_i2c_device('inv_eeprom', 0x55, 2)
        self.insmod('inv_ipmi')
        
        for i in range(0,len(self._path_prefix_list)):
            logging.warning("%s" % self._path_prefix_list[i] )
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
