from onl.platform.base import *
from onl.platform.ingrasys import *
import os
import sys

def msg(s, fatal=False):
    sys.stderr.write(s)
    sys.stderr.flush()
    if fatal:
        sys.exit(1)

class OnlPlatform_x86_64_ingrasys_s9280_64x_r0(OnlPlatformIngrasys):
    PLATFORM='x86-64-ingrasys-s9280-64x-r0'
    MODEL="S9280-64X"
    SYS_OBJECT_ID=".9280.64"
    PORT_COUNT=64
    PORT_CONFIG="64x100"

    def baseconfig(self):

        # fp port to phy port mapping
        fp2phy_array=( 0,  1,  4,  5,  8,  9, 12, 13, 16, 17, 20, 21, 24, 25, 28, 29,
                      32, 33, 36, 37, 40, 41, 44, 45, 48, 49, 52, 53, 56, 57, 60, 61,
                       2,  3,  6,  7, 10, 11, 14, 15, 18, 19, 22, 23, 26, 27, 30, 31,
                      34, 35, 38, 39, 42, 43, 46, 47, 50, 51, 54, 55, 58, 59, 62, 63)
        # fp port to led port mapping
        fp2led_array=( 1,  2,  5,  6,  9, 10, 13, 14,  1,  2,  5,  6,  9, 10, 13, 14,
                       1,  2,  5,  6,  9, 10, 13, 14,  1,  2,  5,  6,  9, 10, 13, 14,
                       3,  4,  7,  8, 11, 12, 15, 16,  3,  4,  7,  8, 11, 12, 15, 16,
                       3,  4,  7,  8, 11, 12, 15, 16,  3,  4,  7,  8, 11, 12, 15, 16)
                
        # vid to mac vdd value mapping 
        vdd_val_array=( 0.85,  0.82,  0.77,  0.87,  0.74,  0.84,  0.79,  0.89 )
        # vid to rov reg value mapping 
        rov_reg_array=( 0x79,  0x73,  0x69,  0x7D,  0x63, 0x77, 0x6D, 0x81 )
        
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
                ('pca9548', 0x71, 19), #pca9548_2
                ('pca9546', 0x71, 20), #pca9546_1
                ('pca9548', 0x75, 0),  #pca9548_11
                ('pca9548', 0x74, 21), #pca9548_3
                ('pca9548', 0x74, 22), #pca9548_4
                ('pca9548', 0x74, 23), #pca9548_5
                ('pca9548', 0x74, 24), #pca9548_6
                ('pca9548', 0x74, 25), #pca9548_7
                ('pca9548', 0x74, 26), #pca9548_8
                ('pca9548', 0x74, 27), #pca9548_9
                ('pca9548', 0x74, 28), #pca9548_10
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
        os.system("i2cset -y -r 0 0x25 6 0xFF")
        os.system("i2cset -y -r 0 0x25 7 0xFF")
        os.system("i2cset -y -r 0 0x25 4 0x00")
        os.system("i2cset -y -r 0 0x25 5 0x00")
       
        # Init BMC RST and SEL IO Expander
        os.system("i2cset -y -r 0 0x26 2 0x3F")
        os.system("i2cset -y -r 0 0x26 3 0x1F")
        os.system("i2cset -y -r 0 0x26 6 0xD0")
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
        os.system("i2cset -y -r 32 0x76 2 0x04")
        os.system("i2cset -y -r 32 0x76 3 0xDF")
        os.system("i2cset -y -r 32 0x76 6 0x09")
        os.system("i2cset -y -r 32 0x76 7 0x3F")
        os.system("i2cset -y -r 32 0x76 4 0x00")
        os.system("i2cset -y -r 32 0x76 5 0x00")

        # _i2c_sensors_init 

        self.new_i2c_devices(
            [
                # w83795, hwmon1
                ('w83795adg', 0x2F, 16),

                # lm75_1 Rear Panel, hwmon2
                ('lm75', 0x4D, 6),

                # lm75_2 Rear MAC, hwmon3
                ('lm75', 0x4E, 6),
                
                # lm86 , hwmon4
                ('lm86', 0x4C, 6),
                
                # lm75_3 Front Panel, hwmon5
                ('lm75', 0x4D, 7),
                
                # lm75_4 Front MAC, hwmon6
                ('lm75', 0x4E, 7),
                
                # tmp75 BMC board thermal, hwmon7
                ('lm75', 0x4A, 16),
                
                # tmp75 CPU board thermal, hwmon8
                ('tmp75', 0x4F, 0),
            ]
        )

        # hwmon9
        #os.system("modprobe jc42")

        # _i2c_cpld_init
        
        self.insmod("ingrasys_s9280_64x_i2c_cpld")

        # add cpld 1~5 to sysfs
        for i in range(1, 6):
            self.new_i2c_device('ingrasys_cpld%d' % i, 0x33, i)

        # _i2c_psu_init

        self.insmod("ingrasys_s9280_64x_psu")

        # add psu 1~2 to sysfs
        for i in range(1, 3):
            self.new_i2c_device('psu%d' % i, 0x50, 19-i)

        # _i2c_qsfp_eeprom_init 
        for i in range(1, 65):
            phy_port = fp2phy_array[i-1] + 1
            port_group = (phy_port-1)/8
            eeprom_busbase = 41 + (port_group * 8)
            eeprom_busshift = (phy_port-1)%8            
            eeprom_bus = eeprom_busbase + eeprom_busshift
            self.new_i2c_device('optoe1', 0x50, eeprom_bus)

        # _i2c_sfp_eeprom_init 
        for i in range(1, 3):
            self.new_i2c_device('sff8436', 0x50, 28+i)

        # _mac_vdd_init
        rov_status_file = open("/sys/bus/i2c/devices/1-0033/cpld_rov_status", "r")
        reg_val_str = rov_status_file.read()
        rov_status_file.close()

        reg_val = int(reg_val_str, 16)
        vid = reg_val & 0x7
        mac_vdd_val = vdd_val_array[vid]
        rov_reg = rov_reg_array[vid]
        
        msg("Setting mac vdd %1.2f with rov register value 0x%x\n" % (mac_vdd_val, rov_reg) ) 
        os.system("i2cset -y -r 15 0x76 0x21 0x%x w" % rov_reg)
        
        # _i2c_fan_speed_init 
        os.system("echo 120 > /sys/class/hwmon/hwmon1/device/pwm2")

        # _util_port_led_clear 
        os.system("i2cset -m 0x04 -y -r 32 0x76 2 0x00")
        os.system("sleep 1")
        os.system("i2cset -m 0x04 -y -r 32 0x76 2 0xFF")

        # turn on sys led
        os.system("i2cset -m 0x80 -y -r 10 0x76 2 0x80")

        return True
        
        
