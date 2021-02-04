from onl.platform.base import *
from onl.platform.accton import *

class OnlPlatform_x86_64_accton_as4630_54pe_r0(OnlPlatformAccton,
                                              OnlPlatformPortConfig_48x1_4x25_2x100):

    PLATFORM='x86-64-accton-as4630-54pe-r0'
    MODEL="AS4630-54PE"
    SYS_OBJECT_ID=".4630.54"

    def baseconfig(self):
        self.insmod('optoe')
        self.insmod('ym2651y')
        for m in [ 'cpld', 'psu', 'leds' ]:
            self.insmod("x86-64-accton-as4630-54pe-%s.ko" % m)

        ########### initialize I2C bus 0 ###########
        # initialize multiplexer (PCA9548)        
        self.new_i2c_device('pca9548', 0x77, 1)
        
        self.new_i2c_devices([
            # initialize multiplexer (PCA9548)                        
            ('pca9548', 0x71, 2),
            ('pca9548', 0x70, 3),
            #initiate CPLD  
            ('as4630_54pe_cpld', 0x60, 3)
            ])
        self.new_i2c_devices([
            # inititate LM77
            ('lm77', 0x48, 14),
            # inititate LM75
            ('lm75', 0x4a, 25),
            ('lm75', 0x4b, 24)
         ])

        self.new_i2c_devices([
            # initiate PSU-1
            ('as4630_54pe_psu1', 0x50, 10),
            ('ype1200am', 0x58, 10),
            # initiate PSU-2
            ('as4630_54pe_psu2', 0x51, 11),
            ('ype1200am', 0x59, 11),
         ])

        # initialize SFP port 49~52
        for port in range(49, 53):
            self.new_i2c_device('optoe2', 0x50, port-31)
            subprocess.call('echo port%d > /sys/bus/i2c/devices/%d-0050/port_name' % (port, port-31), shell=True)
       
        # initialize SFP port 49~52
        for port in range(53, 55):
            self.new_i2c_device('optoe1', 0x50, port-31)
            subprocess.call('echo port%d > /sys/bus/i2c/devices/%d-0050/port_name' % (port, port-31), shell=True)
      
        self.new_i2c_device('24c02', 0x57, 1)
        return True
