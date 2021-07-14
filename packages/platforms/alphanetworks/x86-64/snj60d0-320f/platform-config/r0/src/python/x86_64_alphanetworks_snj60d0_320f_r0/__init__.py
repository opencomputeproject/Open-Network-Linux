from onl.platform.base import *
from onl.platform.alphanetworks import *

class OnlPlatform_x86_64_alphanetworks_snj60d0_320f_r0(OnlPlatformAlphaNetworks,
                                              OnlPlatformPortConfig_32x400_2x10):
    PLATFORM='x86-64-alphanetworks-snj60d0-320f-r0'
    MODEL="SNJ60D0-320F"
    SYS_OBJECT_ID=".6040.8"

    def baseconfig(self):
        self.insmod('snj60d0-320f_onie_eeprom')
        self.insmod('snj60d0-320f_fpga')
        self.insmod('snj60d0-320f_i2c_mux_cpld')

        ########### initialize I2C bus 0 ###########
        self.new_i2c_devices([

            # ONIE EEPROM @MB
            ('snj60d0_onie_eeprom', 0x56, 0),
            
            # FPGA @MB
            ('snj60d0_fpga', 0x5E, 0),

            # CPU Board G751 (Ambient)
            ('lm75', 0x4F, 0),

            # initialize multiplexer (PCA9548 #0)
            ('pca9548', 0x70, 0),		

            ])

        ########### initialize I2C bus PCA9548 #0 ###########
        self.new_i2c_devices(
            [
            # FRU EEPROM @MB
            ('24_c512c', 0x51, 1),

            # CFG EEPROM @MB
            ('24_c02d', 0x51, 2),
                       
            # TMP75#1 (Hot Spot) #use lm75 will cause device busy, use other name
            ('lm75', 0x4D, 3),

            # TMP75#0 (Ambient)  #use lm75 will cause device busy, use other name
            ('lm75', 0x4C, 4),

            # initialize multiplexer (PCA9545 #0)
            ('pca9545', 0x71, 5),

            # reserved
            
            # initialize multiplexer (PCA9548 #1)
            ('pca9548', 0x72, 7),

            # reserved
            
            ])

        ########### initialize I2C bus PCA9545 #0 ###########
        self.new_i2c_devices(
            [
            # PSU #0
            ('24c02', 0x50, 9),
            ('yesm1300am', 0x58, 9),
            # PSU #1
            ('24c02', 0x51, 10),
            ('yesm1300am', 0x59, 10),
            # SFP+ Port 1
            ('optoe2', 0x50, 11),
            # SFP+ Port 0
            ('optoe2', 0x50, 12),
            ])

        ########### initialize I2C bus PCA9548 #1 ###########
        self.new_i2c_devices(
            [
            # Port CPLD 0~3
            ('snj60d0_320f_cpld1' , 0x5F, 13),
            ('snj60d0_320f_cpld2' , 0x5F, 14),
            ('snj60d0_320f_cpld3' , 0x5F, 15),
            ('snj60d0_320f_cpld4' , 0x5F, 16),
            
            # Si clock generator 5345D
            ('si5345d', 0x68, 17),

            # reserved
            # reserved
            
            # initialize multiplexer (PCA9548 #2)
            ('pca9548', 0x73, 20),
            ])        
        
        # Initialize QSFP devices (Port EEPROM)
        for port in range(1, 33):
            self.new_i2c_device('optoe1', 0x50, port+20)

        ########### initialize I2C bus PCA9548 #2 @Fan Expander ###########
        self.new_i2c_devices(
            [
            # Fan EEROM
            ('24_c02', 0x57, 53),
            # Fan EEROM
            ('24_c02', 0x57, 54),
            # Fan EEROM
            ('24_c02', 0x57, 55),
            # Fan EEROM
            ('24_c02', 0x57, 56),
            # Fan EEROM
            ('24_c02', 0x57, 57),
            # Fan EEROM
            ('24_c02', 0x57, 58),
            ])


        return True
