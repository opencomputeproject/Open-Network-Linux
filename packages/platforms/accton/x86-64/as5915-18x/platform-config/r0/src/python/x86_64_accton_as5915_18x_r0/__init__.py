from onl.platform.base import *
from onl.platform.accton import *
from time import sleep

class OnlPlatform_x86_64_accton_as5915_18x_r0(OnlPlatformAccton,
                                              OnlPlatformPortConfig_6x10_8x1_4x1):
    PLATFORM='x86-64-accton-as5915-18x-r0'
    MODEL="AS5915-18X"
    SYS_OBJECT_ID=".5915.18"

    def baseconfig(self):
        os.system("modprobe i2c-ismt")
        os.system("modprobe pmbus_core")
        self.insmod('optoe')
        self.insmod("ym2651y")
        #os.system("insmod /lib/modules/`uname -r`/kernel/drivers/hwmon/pmbus/ucd9000.ko")
        for m in [ 'fpga', 'cpld', 'sys', 'fan', 'leds' ]:
            self.insmod("x86-64-accton-as5915-18x-%s" % m)

        ########### initialize I2C bus 0 ###########
        self.new_i2c_devices(
            [
                # initialize multiplexer (PCA9548)
                ('pca9548', 0x72, 0), # i2c  2-9
                ('pca9548', 0x70, 2), # i2c 10-17
                ('pca9548', 0x71, 3), # i2c 18-25
                ]
            )

        # initialize QSFP devices
        for port in range(1, 7):
            self.new_i2c_device('optoe2', 0x50, port+9)
            subprocess.call('echo port%d > /sys/bus/i2c/devices/%d-0050/port_name' % (port, port+9), shell=True)

        # initialize SFP devices
        for port in range(7, 15):
            self.new_i2c_device('optoe2', 0x50, port+11)
            subprocess.call('echo port%d > /sys/bus/i2c/devices/%d-0050/port_name' % (port, port+11), shell=True)

        ########### initialize I2C bus 1 ###########
        self.new_i2c_devices(
            [
                # initialize multiplexer (PCA9548)
                ('pca9548', 0x76, 1),  # i2c 26-33
                ('pca9548', 0x74, 27), # i2c 34-41

                # initialize UCD90160
                ('ucd90160', 0x34, 35),

                # initialize CPLD
                ('as5915_18x_fpga', 0x64, 31), # i2c 42-45
                ('as5915_18x_cpld', 0x63, 32),
                ('as5915_18x_fan',  0x66, 32),

                # initiate PSU-1 DC Power
                ('ym2401',  0x5a, 36),

                # initiate PSU-2 DC Power
                ('ym2401',  0x5b, 36),

                # initiate IDPROM
                ('as5915_18x_sys', 0x56, 1),
                ]
            )

        # sleep for a while to make sure the mux channels of fpga are created
        sleep(0.1)
        self.new_i2c_devices(
            [
                # inititate LM75
                ('lm75', 0x49, 43),
                ('lm75', 0x4a, 44),
                ('lm75', 0x4b, 45),
                ('lm75', 0x4c, 45),
                ]
            )

        # Write sensors.conf for ucd90160
        lines = """
        bus "i2c-35" "i2c-27-mux (chan_id 1)"
            chip "ucd90160-i2c-*-34"
                label in1  "H_VCC12_MON"
                label in2  "VCC12_MON"
                label in3  "USB_VCC5P0_MON"
                label in4  "VCC5P0_MON"
                label in5  "VDD3P3_MON"
                label in6  "VDD1P8_MON"
                label in7  "VDD1P0_MON"
                label in8  "SFP_3P3_MON"
                label in9  "VDD1P0_ROV_MON"
                label in10 "VPP_DDR2P5_MON"
                label in11 "VDD1P2_DDR_MON"
                label in12 "VDD0P6_MON"
                label in13 "Vmonitor_3V3_MON"
                label in14 "FPGA_2V5_MON"
                label in15 "FPGA_1V2_MON"
                label in16 "MANG_2P5_MON"
        """
        with open("/etc/sensors.d/sensors.conf1","w+") as f:
            f.write(lines)

        return True
