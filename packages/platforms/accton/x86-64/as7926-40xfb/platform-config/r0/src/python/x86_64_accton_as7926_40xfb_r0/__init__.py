from onl.platform.base import *
from onl.platform.accton import *
from time import sleep

class OnlPlatform_x86_64_accton_as7926_40xfb_r0(OnlPlatformAccton,
                                              OnlPlatformPortConfig_48x1_4x10):
    PLATFORM='x86-64-accton-as7926-40xfb-r0'
    MODEL="AS7926-40XFB"
    SYS_OBJECT_ID=".7926.40"

    def baseconfig(self):
        self.insmod('optoe')
        for m in [ 'cpld', 'fan', 'psu', 'leds', 'thermal' ]:
            self.insmod("x86-64-accton-as7926-40xfb-%s.ko" % m)

        ########### initialize I2C bus 0 ###########
        self.new_i2c_devices([
                # initialize multiplexer (PCA9548)
                ('pca9548', 0x77, 0), # i2c 1-8

                # initiate  multiplexer (PCA9548) of bottom board
                ('pca9548', 0x76, 1), # i2c 9-16
                ('pca9548', 0x72, 2), # i2c 17-24
                ('pca9548', 0x73, 9), # i2c 25-32

                # initiate multiplexer for QSFP ports
                ('pca9548', 0x74, 25), # i2c 33-40
                ('pca9548', 0x74, 26), # i2c 41-48
                ('pca9548', 0x74, 27), # i2c 49-56
                ('pca9548', 0x74, 28), # i2c 57-64
                ('pca9548', 0x74, 29), # i2c 65-72

                # initiate multiplexer for FAB ports
                ('pca9548', 0x70, 1),  # i2c 73-80
                ('pca9548', 0x74, 73), # i2c 81-88
                ('pca9548', 0x74, 74), # i2c 89-96

                #initiate CPLD
                ('as7926_40xfb_cpld2', 0x62, 12),
                ('as7926_40xfb_cpld3', 0x63, 13),
                ('as7926_40xfb_cpld4', 0x64, 76),

                ('24c02', 0x57, 0),
                ])

        # initialize QSFP port(0-39), FAB port(40-52)
        port_i2c_bus = [33, 34, 37, 38, 41, 42, 45, 46, 49, 50,
                        53, 54, 57, 58, 61, 62, 65, 66, 69, 70,
                        35, 36, 39, 40, 43, 44, 47, 48, 51, 52,
                        55, 56, 59, 60, 63, 64, 67, 68, 71, 72,
                        93, 84, 83, 82, 81, 86, 85, 88, 87, 90,
                        89, 92, 91]

        for port in range(0, 53):
            self.new_i2c_device('optoe1', 0x50, port_i2c_bus[port])
            subprocess.call('echo port%d > /sys/bus/i2c/devices/%d-0050/port_name' % (port, port_i2c_bus[port]), shell=True)

        # initialize SFP port 53-54
        self.new_i2c_device('optoe2', 0x50, 30)
        subprocess.call('echo port53 > /sys/bus/i2c/devices/30-0050/port_name', shell=True)
        self.new_i2c_device('optoe2', 0x50, 31)
        subprocess.call('echo port54 > /sys/bus/i2c/devices/31-0050/port_name', shell=True)

        return True
