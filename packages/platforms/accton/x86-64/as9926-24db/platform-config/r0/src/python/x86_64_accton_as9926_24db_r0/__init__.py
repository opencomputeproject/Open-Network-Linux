from onl.platform.base import *
from onl.platform.accton import *

class OnlPlatform_x86_64_accton_as9926_24db_r0(OnlPlatformAccton,
                                               OnlPlatformPortConfig_24x400_2x10):
    PLATFORM='x86-64-accton-as9926-24db-r0'
    MODEL="AS9926-24DB"
    SYS_OBJECT_ID=".9926.24"

    def baseconfig(self):
        self.insmod('optoe')

        for m in [ 'fan', 'psu', 'leds', 'sfp', 'sys', 'thermal' ]:
            self.insmod("x86-64-accton-as9926-24db-%s.ko" % m)

        ########### initialize I2C bus 0 ###########
        self.new_i2c_devices(
            [
                # initialize multiplexer (PCA9548)
                ('pca9548', 0x77, 0),
                ('pca9548', 0x72, 2),
                ('pca9548', 0x73, 2),
                ('pca9548', 0x74, 2),
                ('pca9548', 0x76, 2),
                ]
            )

        # initialize QSFP devices
        for port in range(1, 25):
            self.new_i2c_device('optoe1', 0x50, port+8)	

        # initialize SFP devices
        for port in range(25, 27):
            self.new_i2c_device('optoe2', 0x50, port+8)

        # set port name
        for port in range(1, 27):
            subprocess.call('echo port%d > /sys/bus/i2c/devices/%d-0050/port_name' % (port, port+8), shell=True)


        return True

