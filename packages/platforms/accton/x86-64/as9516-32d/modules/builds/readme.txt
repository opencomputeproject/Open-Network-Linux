***   BF FPGA I2C DRIVER ***

bf_fpga implements multi controller i2c automation besides some other non-i2c
functions.

bf_fpga/ files implement the driver for bf_fpga, mainly the i2c operations.
This file explains the code organization of the driver.

The code is organized such that the driver can be built for linux kernel
or fully in linux user space. A porting layer is defined for this purpose and
implemented for linux kernel use case.

bf_fpga-|
        | bf_fpga_main.c                /* kernel driver main */
        | bf_fpga_ioctl.c               /* kernel ioctl */
        | bf_fpga_sysfs.c               /* kernel syfs */
        | bf_fpga_ioctl.h               /* ioctl header, public */
        | bf_fpga_priv.h                /* kernel driveer, private */
        | i2c - |
                | bf_fpga_i2c_ctrl.c    /* i2c control operations */
                | bf_fpga_i2c.c         /* i2c operations */
                | bf_fpga_i2c.h         /* i2c public header file */
                | bf_fpga_i2c_porting.c /* porting layer, private */
                | bf_fpga_i2c_porting.h /* porting layer, private */
                | bf_fpga_i2c_reg.h     /* fpga registers definitions */
                | bf_fpga_i2c_priv.h    /* i2c driver private header */


Kernel implementation use case:
===============================

     user1             user2          user3
      |                 |              |
    ioctl()            ioctl()        sysfs
      |                 |              |
bf_fpga_ioctl.c    bf_fpga_ioctl.c   bf_fpga_sysfs.c
      |                 |              |
----------------------------------------------------------
             |
         bf_fpga_i2c_ctrl.c --|
          bf_fpga_i2c.c   ----|----bf_fpga_i2c_porting.c (kernel version)



 full user space implementation use case:
=========================================

     user1i(main)      user2          user3
      |                  |              |
      |                  |              |
    bf_fpga_lib  <-----(RPC)-----------(RPC) 
             |
             |
         bf_fpga_i2c_ctrl.c --|
          bf_fpga_i2c.c   ----|----bf_fpga_i2c_porting.c (user space version)

sysfs usage
============
The two fixed sysfs files are created when this driver module is loaded,
"new_device" and "remove_device", at /sys/class/bf/bf_fpga_<n>/device, n being
the fpga index.

# ls /sys/class/bf/bf_fpga_0/device
bf
...
new_device
...
remove_device


platform specific i2c devices can be added and removed using these two fixed
files, respectively.

Use following command with root previleges to add an i2c device sysfs file,
namely "xyz" on fpga bus <bus_id> at i2c address "i2c_addr".

cd /sys/class/bf/bf_fpga_<n>/device
echo xyz <bus_id> <i2c_addr in hex>  [max read size] > new_device

e.g., to add qsfp-dd-3 on bus 3 with i2c_adress=0x50:
# cd /sys/class/bf/bf_fpga_0/device
#echo qsfp-dd-3 3 0x50 > new_device

another example to add an eeprom on bus 32 at address 0x51:
#echo eeprom 32 0x51 > new_device

** No second device on the same bus with the same i2c address can exist **
** "max read size" must be <= PAGE_SIZE. defaults to 1 if absent **

To remove the sysfs file for an i2c device already added earlier,
# echo <bus_id> <i2c address in hex> > remove_device
#echo 32 0x51 > remove_device   /* this removes the device file, eeprom */

/* how to perform i2c read/write on newly created sysfs i2c file */
write operation : use write() system calls on newly created sysfs file.

read operation: use read() system call on newly created sysfs file.
It would return <max read size> or PAGE_SIZE number of bytes, whatever is less.

****** Note that created sysfs files cannot be used for address-read type of
i2c transactions that involve a write followed by repeat start followed by a
read transfer.
