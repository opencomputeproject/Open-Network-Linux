from onl.platform.base import *
from onl.platform.ingrasys import *
import os

class OnlPlatform_x86_64_ingrasys_s9230_64x_r0(OnlPlatformIngrasys):
    PLATFORM='x86-64-ingrasys-s9230-64x-r0'
    MODEL="S9230-64X"
    SYS_OBJECT_ID=".9230.64"
    PORT_COUNT=64
    PORT_CONFIG="64x100"

    def baseconfig(self):

        # fp port to phy port mapping
        fp2phy_array=( 1,  2,  5,  6,  9, 10, 13, 14, 17, 18, 21, 22, 25, 26, 29, 30, 
                                33, 34, 37, 38, 41, 42, 45, 46, 49, 50, 53, 54, 57, 58, 61, 62, 
                                  3,  4,  7,  8, 11, 12, 15, 16, 19, 20, 23, 24, 27, 28, 31, 32, 
                                35, 36, 39, 40, 43, 44, 47, 48, 51, 52, 55, 56, 59, 60, 63, 64)
                
        self.insmod("eeprom_mb") 
        # init SYS EEPROM devices
        self.new_i2c_devices(
            [
                # _i2c_mb_eeprom_init 
                ('mb_eeprom', 0x55, 0),

                # _i2c_cb_eeprom_init  
                ('mb_eeprom', 0x51, 0),
            ]
        )
        
        os.system("modprobe w83795")
        os.system("modprobe eeprom")
        os.system("modprobe gpio_pca953x")
        self.insmod("optoe") 
        
        ########### initialize I2C bus 0 ###########
        # init PCA9548
        self.new_i2c_devices(
            [
                ('pca9548', 0x70, 0),  #pca9548_0
                ('pca9548', 0x73, 0),  #pca9548_1
                ('pca9546', 0x72, 0),  #pca9546_0
                ('pca9546', 0x71, 20), #pca9546_1
                ('pca9548', 0x71, 19), #pca9548_2
                ('pca9548', 0x74, 25), #pca9548_3
                ('pca9548', 0x74, 26), #pca9548_4
                ('pca9548', 0x74, 27), #pca9548_5
                ('pca9548', 0x74, 28), #pca9548_6
                ('pca9548', 0x74, 29), #pca9548_7
                ('pca9548', 0x74, 30), #pca9548_8
                ('pca9548', 0x74, 31), #pca9548_9
                ('pca9548', 0x74, 32), #pca9548_10
            ]
        )
        # _i2c_hwm_init  
        os.system("i2cset -y -r 16 0x2f 0x00 0x80")
        os.system("i2cset -y -r 16 0x2f 0x01 0x9C")
        os.system("i2cset -y -r 16 0x2f 0x04 0x00")
        os.system("i2cset -y -r 16 0x2f 0x06 0xFF")
        os.system("i2cset -y -r 16 0x2f 0x07 0x00")
        os.system("i2cset -y -r 16 0x2f 0x01 0x1C")
        os.system("i2cset -y -r 16 0x2f 0x00 0x82")
        os.system("i2cset -y -r 16 0x2f 0x0F 0x00")
        os.system("i2cset -y -r 16 0x2f 0x18 0x84")
        os.system("i2cset -y -r 16 0x2f 0x19 0x84")
        
        # _i2c_io_exp_init 
        # need to init BMC io expander first due to some io expander are reset default
        # Init BMC INT & HW ID IO Expander
        os.system("i2cset -y -r 0 0x24 6 0xFF")
        os.system("i2cset -y -r 0 0x24 7 0xFF")
        os.system("i2cset -y -r 0 0x24 4 0x00")
        os.system("i2cset -y -r 0 0x24 5 0x00")

        # Init BMC PSU status IO Expander
        os.system("i2cset -y -r 0 0x25 2 0x00")
        os.system("i2cset -y -r 0 0x25 3 0x00")
        os.system("i2cset -y -r 0 0x25 6 0xDB")
        os.system("i2cset -y -r 0 0x25 7 0xE3")
        os.system("i2cset -y -r 0 0x25 4 0x00")
        os.system("i2cset -y -r 0 0x25 5 0x00")
       
        # Init BMC RST and SEL IO Expander
        os.system("i2cset -y -r 0 0x26 2 0x3F")
        os.system("i2cset -y -r 0 0x26 3 0x1F")
        os.system("i2cset -y -r 0 0x26 6 0xC0")
        os.system("i2cset -y -r 0 0x26 7 0x00")
        os.system("i2cset -y -r 0 0x26 4 0x00")
        os.system("i2cset -y -r 0 0x26 5 0x00")

        # Init System LED & HW ID IO Expander
        os.system("i2cset -y -r 10 0x76 2 0x00")
        os.system("i2cset -y -r 10 0x76 6 0x00")
        os.system("i2cset -y -r 10 0x76 7 0xFF")
        os.system("i2cset -y -r 10 0x76 4 0x00")
        os.system("i2cset -y -r 10 0x76 5 0x00")

        # Init FAN Board Status IO Expander
        os.system("i2cset -y -r 0 0x20 2 0x11")
        os.system("i2cset -y -r 0 0x20 3 0x11")
        os.system("i2cset -y -r 0 0x20 6 0xCC")
        os.system("i2cset -y -r 0 0x20 7 0xCC")
        os.system("i2cset -y -r 0 0x20 4 0x00")
        os.system("i2cset -y -r 0 0x20 5 0x00")

        # Init System SEL and RST IO Expander
        os.system("i2cset -y -r 11 0x76 2 0x0F")
        os.system("i2cset -y -r 11 0x76 3 0xDF")
        os.system("i2cset -y -r 11 0x76 6 0x09")
        os.system("i2cset -y -r 11 0x76 7 0x1F")
        os.system("i2cset -y -r 11 0x76 4 0x00")
        os.system("i2cset -y -r 11 0x76 5 0x00")

        # Init CPU CPLD IO Expander
        os.system("i2cset -y -r 0 0x77 6 0xFF")
        os.system("i2cset -y -r 0 0x77 7 0xFF")
        os.system("i2cset -y -r 0 0x77 4 0x00")
        os.system("i2cset -y -r 0 0x77 5 0x00")

        # _i2c_sensors_init 

        self.new_i2c_devices(
            [
                # lm75_1 Rear Panel, hwmon1
                ('lm75', 0x4D, 6),

                # lm75_2 Rear MAC, hwmon2
                ('lm75', 0x4E, 6),
                
                # lm75_3 Front Panel, hwmon3
                ('lm75', 0x4D, 7),
                
                # lm75_4 Front MAC, hwmon4
                ('lm75', 0x4E, 7),
                
                # tmp75 BMC board thermal, hwmon5
                ('tmp75', 0x4A, 16),
                
                # tmp75 CPU board thermal, hwmon6
                ('tmp75', 0x4F, 0),

                # w83795, hwmon7
                ('w83795adg', 0x2F, 16),

            ]
        )

        # _i2c_cpld_init       
        self.insmod("ingrasys_s9230_64x_i2c_cpld")

        # add cpld 1~5 to sysfs
        for i in range(1, 6):
            self.new_i2c_device('ingrasys_cpld%d' % i, 0x33, i)

        # _i2c_psu_init

        self.insmod("ingrasys_s9230_64x_psu")

        # add psu 1~2 to sysfs
        for i in range(1, 3):
            self.new_i2c_device('psu%d' % i, 0x50, 19-i)

        # _i2c_qsfp_eeprom_init 
        for i in range(1, 65):
            phy_port = fp2phy_array[i-1]
            port_group = (phy_port-1)/8
            eeprom_busbase = 33 + (port_group * 8)
            eeprom_busshift = (phy_port-1)%8            
            eeprom_bus = eeprom_busbase + eeprom_busshift
            self.new_i2c_device('optoe1', 0x50, eeprom_bus)

        # _i2c_sfp_eeprom_init 
        for i in range(1, 3):
            self.new_i2c_device('sff8436', 0x50, 20+i)

        # _i2c_fan_speed_init 
        os.system("echo 120 > /sys/class/hwmon/hwmon7/device/pwm2")

        # set 10gmux to cpu
        os.system("i2cset -m 0xff -y -r 13 0x67 0x06 0x18")
        os.system("i2cset -m 0xff -y -r 13 0x67 0x2c 0x00")
        os.system("i2cset -m 0xff -y -r 13 0x67 0x35 0x80")
        os.system("i2cset -m 0xff -y -r 13 0x67 0x34 0xab")
        os.system("i2cset -m 0xff -y -r 13 0x67 0x16 0x01")
        os.system("i2cset -m 0xff -y -r 13 0x67 0x18 0x80")
        os.system("i2cset -m 0xff -y -r 13 0x67 0x17 0xab")
        os.system("i2cset -m 0xff -y -r 13 0x67 0x3a 0x00")
        os.system("i2cset -m 0xff -y -r 13 0x67 0x41 0x00")
        os.system("i2cset -m 0xff -y -r 13 0x67 0x43 0x80")
        os.system("i2cset -m 0xff -y -r 13 0x67 0x42 0xab")
        os.system("i2cset -m 0xff -y -r 13 0x67 0x24 0x01")
        os.system("i2cset -m 0xff -y -r 13 0x67 0x26 0x80")
        os.system("i2cset -m 0xff -y -r 13 0x67 0x25 0xab")
        os.system("echo 0xf3 > /sys/class/i2c-dev/i2c-1/device/1-0033/cpld_10gmux_config")

        # turn on sys led
        os.system("i2cset -m 0x80 -y -r 10 0x76 2 0x80")

        return True
        
        
