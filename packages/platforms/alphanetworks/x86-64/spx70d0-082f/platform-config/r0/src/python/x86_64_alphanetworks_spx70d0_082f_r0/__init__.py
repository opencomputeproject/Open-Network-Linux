from onl.platform.base import *
from onl.platform.alphanetworks import *

class OnlPlatform_x86_64_alphanetworks_spx70d0_082f_r0(OnlPlatformAlphaNetworks,
                                              OnlPlatformPortConfig_8x10_2x100):
    PLATFORM='x86-64-alphanetworks-spx70d0-082f-r0'
    MODEL="SPX70D0-082F"
    SYS_OBJECT_ID=".6040.10"

    def baseconfig(self):
        ########### initialize I2C bus 0 ###########
        self.new_i2c_devices([

            # TPS53647 @MB
            ('tps53647', 0x65, 0),

            # 8A34002 @MB
            ('8A34002', 0x5B, 0),

            # ONIE EEPROM @MB (FM24C128A)
            ('fm24c128a', 0x56, 0),
			
            # PCA9539#2 @MB
            ('pca9539', 0x75, 0),
			
            # initialize multiplexer (PCA9548 #0)
            ('pca9548', 0x70, 0),

            ])

        ########### initialize I2C bus PCA9548 #0 ###########
        self.new_i2c_devices(
            [
            # CFG EEPROM @MB (AT24C02D)
            ('at24c02d', 0x51, 1),
                      
            # initialize multiplexer (PCA9548 #2)
            ('pca9548', 0x73, 3),
                      
            # MAC @MB
            ('bcm88470', 0x47, 4),

            # PSU #0
            ('psu', 0x58, 5),
			
            # initialize multiplexer (PCA9548 #1)
            ('pca9548', 0x72, 7),
			
            # PCA9539#0 @MB
            ('pca9539', 0x76, 8),
			
            ])

        ########### initialize I2C bus PCA9548 #2 ###########
        self.new_i2c_devices(
            [
            # QSFP28 1 uplink
            ('spx70d0_qsfp1', 0x50, 9),

            # QSFP28 2 uplink
            ('spx70d0_qsfp2', 0x50, 10),

            # PCA9539#3 @MB
            ('pca9539', 0x77, 11),

            # XFP 7 downlink
            ('spx70d0_xfp7', 0x50, 12),

            # XFP 8 downlink
            ('spx70d0_xfp8', 0x50, 13),
            
            # XFP 9 downlink
            ('spx70d0_xfp9', 0x50, 14),

            # XFP 10 downlink
            ('spx70d0_xfp10', 0x50, 15),

            # PCA9539#4 @MB
            ('pca9539', 0x74, 16),
            
            ])

        ########### initialize I2C bus PCA9548 #1 ###########
        self.new_i2c_devices(
            [
            # XFP 1 downlink
            ('spx70d0_xfp1', 0x50, 17),

            # XFP 2 downlink
            ('spx70d0_xfp2', 0x50, 18),
            
            # XFP 3 downlink
            ('spx70d0_xfp3', 0x50, 19),

            # XFP 4 downlink
            ('spx70d0_xfp4', 0x50, 20),

            # PCA9539#1 @MB
            ('pca9539', 0x76, 21),
            
            # reserved
            
            ])

        return True
