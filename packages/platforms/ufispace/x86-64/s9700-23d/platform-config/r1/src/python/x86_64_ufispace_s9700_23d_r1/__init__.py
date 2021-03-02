from onl.platform.base import *
from onl.platform.ufispace import *
from struct import *
from ctypes import c_int, sizeof
import os
import sys
import subprocess
import time
import fcntl

def msg(s, fatal=False):
    sys.stderr.write(s)
    sys.stderr.flush()
    if fatal:
        sys.exit(1)

class IPMI_Ioctl(object):
    _IONONE = 0
    _IOWRITE = 1
    _IOREAD = 2

    IPMI_MAINTENANCE_MODE_AUTO = 0
    IPMI_MAINTENANCE_MODE_OFF  = 1
    IPMI_MAINTENANCE_MODE_ON   = 2

    IPMICTL_GET_MAINTENANCE_MODE_CMD = _IOREAD << 30 | sizeof(c_int) << 16 | \
        ord('i') << 8 | 30  # from ipmi.h
    IPMICTL_SET_MAINTENANCE_MODE_CMD = _IOWRITE << 30 | sizeof(c_int) << 16 | \
        ord('i') << 8 | 31  # from ipmi.h

    def __init__(self):
        self.ipmidev = None
        devnodes=["/dev/ipmi0", "/dev/ipmi/0", "/dev/ipmidev/0"]
        for dev in devnodes:
            try:
                self.ipmidev = open(dev, 'rw')
                break
            except Exception as e:
                print("open file {} failed, error: {}".format(dev, e))

    def __del__(self):
        if self.ipmidev is not None:
            self.ipmidev.close()

    def get_ipmi_maintenance_mode(self):
        input_buffer=pack('i',0)
        out_buffer=fcntl.ioctl(self.ipmidev, self.IPMICTL_GET_MAINTENANCE_MODE_CMD, input_buffer)
        maintanence_mode=unpack('i',out_buffer)[0]

        return maintanence_mode

    def set_ipmi_maintenance_mode(self, mode):
        fcntl.ioctl(self.ipmidev, self.IPMICTL_SET_MAINTENANCE_MODE_CMD, c_int(mode))

class OnlPlatform_x86_64_ufispace_s9700_23d_r1(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9700-23d-r1'
    MODEL="S9700-23D"
    SYS_OBJECT_ID=".9700.23"
    PORT_COUNT=23
    PORT_CONFIG="10x400 + 13x400"
     
    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self): 
        sysfs_mux_reset = "/sys/devices/platform/x86_64_ufispace_s9700_23d_lpc/cpu_cpld/mux_reset"

        # Check I2C status
        retcode = os.system("i2cget -f -y 0 0x75 > /dev/null 2>&1")
        if retcode != 0:

            #read mux failed, i2c bus may be stuck
            msg("Warning: Read I2C Mux Failed!! (ret=%d)\n" % (retcode) )

            #Recovery I2C
            if os.path.exists(sysfs_mux_reset):
                with open(sysfs_mux_reset, "w") as f:
                    #write 0 to sysfs
                    f.write("{}".format(0))
                    msg("I2C bus recovery done.\n")
            else:
                msg("Warning: I2C recovery sysfs does not exist!! (path=%s)\n" % (sysfs_mux_reset) )

    def baseconfig(self):

        # lpc driver
        self.insmod("x86-64-ufispace-s9700-23d-lpc")

        # check i2c bus status
        self.check_i2c_status()

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))
        
        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)
        
        ########### initialize I2C bus 0 ###########
        # init PCA9548
        self.new_i2c_devices(
            [
                ('pca9548', 0x75, 0),
                ('pca9548', 0x72, 0),
                ('pca9548', 0x73, 0),
                ('pca9548', 0x76, 9),
                ('pca9548', 0x76, 10),
                ('pca9548', 0x76, 15),
                ('pca9548', 0x76, 16),
            ]
        )

        self.insmod("x86-64-ufispace-eeprom-mb")
        self.insmod("optoe")

        # init SYS EEPROM devices
        self.new_i2c_devices(
            [
                #  on cpu board
                ('mb_eeprom', 0x57, 0),
            ]
        )

        # init QSFPDD NIF EEPROM
        for port in range(25, 35):
            self.new_i2c_device('optoe1', 0x50, port)

        # init QSFPDD FAB EEPROM
        for port in range(41, 54):
            self.new_i2c_device('optoe1', 0x50, port)

        # init Temperature
        self.new_i2c_devices(
            [               
                # CPU Board Temp
                ('tmp75', 0x4F, 0),
            ]
        )

        # init GPIO sysfs
        #9539_HOST_GPIO_I2C
        self.new_i2c_device('pca9539', 0x74, 0)
        #9555_BEACON_LED
        self.new_i2c_device('pca9535', 0x20, 7)
        #9555_BOARD_ID
        self.new_i2c_device('pca9535', 0x20, 3)
        #9539_VOL_MARGIN
        self.new_i2c_device('pca9535', 0x76, 6)
        #9539_CPU_I2C
        self.new_i2c_device('pca9535', 0x77, 0)

        # export GPIO
        for i in range(432, 512):
            os.system("echo {} > /sys/class/gpio/export".format(i))

        # init GPIO direction
        # 9539_HOST_GPIO_I2C 0x74
        os.system("echo high > /sys/class/gpio/gpio511/direction")
        os.system("echo high > /sys/class/gpio/gpio510/direction")
        os.system("echo in > /sys/class/gpio/gpio509/direction")
        os.system("echo in > /sys/class/gpio/gpio508/direction")
        os.system("echo in > /sys/class/gpio/gpio507/direction")
        os.system("echo in > /sys/class/gpio/gpio506/direction")
        os.system("echo in > /sys/class/gpio/gpio505/direction")
        os.system("echo in > /sys/class/gpio/gpio504/direction")
        os.system("echo in > /sys/class/gpio/gpio503/direction")
        os.system("echo in > /sys/class/gpio/gpio502/direction")
        os.system("echo in > /sys/class/gpio/gpio501/direction")
        os.system("echo low > /sys/class/gpio/gpio500/direction")
        os.system("echo low > /sys/class/gpio/gpio499/direction")
        os.system("echo high > /sys/class/gpio/gpio498/direction")
        os.system("echo in > /sys/class/gpio/gpio497/direction")
        os.system("echo high > /sys/class/gpio/gpio496/direction")

        # init GPIO direction
        # 9555_BEACON_LED 0x20
        os.system("echo in > /sys/class/gpio/gpio495/direction")
        os.system("echo high > /sys/class/gpio/gpio494/direction")
        os.system("echo low > /sys/class/gpio/gpio493/direction")
        os.system("echo low > /sys/class/gpio/gpio492/direction")
        os.system("echo low > /sys/class/gpio/gpio491/direction")
        os.system("echo low > /sys/class/gpio/gpio490/direction")
        os.system("echo low > /sys/class/gpio/gpio489/direction")
        os.system("echo low > /sys/class/gpio/gpio488/direction")
        os.system("echo in > /sys/class/gpio/gpio487/direction")
        os.system("echo low > /sys/class/gpio/gpio486/direction")
        os.system("echo low > /sys/class/gpio/gpio485/direction")
        os.system("echo high > /sys/class/gpio/gpio484/direction")
        os.system("echo low > /sys/class/gpio/gpio483/direction")
        os.system("echo low > /sys/class/gpio/gpio482/direction")
        os.system("echo low > /sys/class/gpio/gpio481/direction")
        os.system("echo low > /sys/class/gpio/gpio480/direction")

        # init GPIO direction
        # 9555_BOARD_ID 0x20, 9539_VOL_MARGIN 0x76, 9539_CPU_I2C 0x77
        for i in range(432, 480):
            os.system("echo in > /sys/class/gpio/gpio{}/direction".format(i))

        #CPLD
        self.insmod("x86-64-ufispace-s9700-23d-cpld")
        for i, addr in enumerate((0x30, 0x31, 0x32)):
            self.new_i2c_device("s9700_23d_cpld" + str(i+1), addr, 1)

        #config mac rov
        
        cpld_addr=[30]
        cpld_bus=1
        rov_addr=0x76
        rov_reg=0x21
        rov_bus=[4]
        
        # vid to mac vdd value mapping 
        vdd_val_array=( 0.82,  0.82,  0.76,  0.78,  0.80,  0.84,  0.86,  0.88 )
        # vid to rov reg value mapping 
        rov_reg_array=( 0x73, 0x73, 0x67, 0x6b, 0x6f, 0x77, 0x7b, 0x7f )
        
        for index, cpld in enumerate(cpld_addr):
            #get rov from cpld
            reg_val_str = subprocess.check_output("cat /sys/bus/i2c/devices/{}-00{}/cpld_psu_status_0".format(cpld_bus, cpld), shell=True)
            reg_val = int(reg_val_str, 16)
            vid = (reg_val & 0xe) >> 1
            mac_vdd_val = vdd_val_array[vid]
            rov_reg_val = rov_reg_array[vid]            
            #set rov to mac
            msg("Setting mac vdd %1.2f with rov register value 0x%x\n" % (mac_vdd_val, rov_reg_val) )
            os.system("i2cset -y {} {} {} {} w".format(rov_bus[index], rov_addr, rov_reg, rov_reg_val))            

        # onie syseeprom
        self.insmod("x86-64-ufispace-s9700-23d-onie-syseeprom.ko")

        self.enable_ipmi_maintenance_mode()

        return True

    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()
            
        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("Current IPMI_MAINTENANCE_MODE=%d\n" % (mode) )
            
        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)
            
        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("After IPMI_IOCTL IPMI_MAINTENANCE_MODE=%d\n" % (mode) )
