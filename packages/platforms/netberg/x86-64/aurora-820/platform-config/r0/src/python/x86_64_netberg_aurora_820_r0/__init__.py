from onl.platform.base import *
import logging

from onl.platform.base import *

class OnlPlatform_x86_64_netberg_aurora_820_r0(OnlPlatformBase,
                                              OnlPlatformPortConfig_32x400):
    MANUFACTURER='Netberg'
    PRIVATE_ENTERPRISE_NUMBER=50424
    PLATFORM='x86-64-netberg-aurora-820-r0'
    MODEL="NBA820"
    SYS_OBJECT_ID=".820.1"
    CHASSIS_FAN_NUM=6
    FAN_VPD_CHANNEL=3
    FAN_VPD_ADDR_BASE=0x52

    _path_prefix_list=[
        "/sys/bus/i2c/devices/2-0058/hwmon/",
        "/sys/bus/i2c/devices/2-0059/hwmon/",
        "/sys/devices/platform/coretemp.0/hwmon/",
        "/sys/bus/i2c/devices/3-0018/hwmon/",
        "/sys/bus/i2c/devices/3-0048/hwmon/",
        "/sys/bus/i2c/devices/3-0049/hwmon/",
        "/sys/bus/i2c/devices/3-004a/hwmon/",
        "/sys/bus/i2c/devices/3-004d/hwmon/",
        "/sys/bus/i2c/devices/3-004e/hwmon/"
    ]
    _path_dst_list=[
        "/var/psu1",
        "/var/psu2",
        "/var/coretemp",
        "/var/board_thermal_1",
        "/var/board_thermal_2",
        "/var/board_thermal_3",
        "/var/board_thermal_4",
        "/var/board_thermal_5",
        "/var/board_thermal_6",
    ]

    def baseconfig(self):
        os.system("insmod /lib/modules/`uname -r`/kernel/drivers/gpio/gpio-ich.ko gpiobase=0")
        self.insmod('i2c-gpio')
        self.insmod('net_ucd90160')
        self.insmod('net-i2c-mux-pca9641')
        self.insmod('net_psu')
        self.insmod('net_cpld')
        self.insmod('net_platform')
        self.insmod('net_eeprom')
        self.new_i2c_device('net_eeprom', 0x55, 2)

        for addr_offset in range(0,self.CHASSIS_FAN_NUM):
            self.new_i2c_device('net_eeprom',self.FAN_VPD_ADDR_BASE+addr_offset,self.FAN_VPD_CHANNEL)

        self.insmod('net_sff')
        self.insmod('vpd')
        
        self.insmod('optoe')
        for ch  in range(0,32):
            self.new_i2c_device('optoe1', 0x50, 14 + ch )

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
