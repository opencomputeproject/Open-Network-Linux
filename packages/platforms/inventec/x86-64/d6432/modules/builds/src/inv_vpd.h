/*************************************************************************
 * 
 *  inv_vpd.h
 *
 *  2018 Inventec Corporation
 *  All Rights Reserved.
 * 
 * NOTICE:  All information contained herein is, and remains
 * the property of Inventec Corp. and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to Inventec Corporation and its suppliers 
 * and may be covered by U.S. and Foreign Patents, patents in process, 
 * and are protected by trade secret or copyright law.
 *
 ************************************************************************/

#ifndef INV_VPD_H
#define INV_VPD_H

#define EEPROM_CLASS   "eeprom"
#define VPD_DEVICE     "vpd"
#define VPD_AUTHOR     "Neil <liao.neil@inventec.com>"
#define VPD_DESC       "Inventec eeprom vpd driver"
#define VPD_VERSION    "1.0.0"
#define VPD_LICENSE    "GPL"

#define VPD_ENTRY_SIZE        (17)
#define VPD_I2C_BUS           (2)
#define VPD_I2C_ADDR          (0x55)

#define VPD_KERN_VER_AF_3_10  (1)

struct register_attr {
struct device_attribute *attr;
char * errmsg;
};

struct vpd_device_attribute{
    struct device_attribute dev_attr;
    int index;
};

#define to_vpd_dev_attr(_dev_attr) \
    container_of(_dev_attr, struct vpd_device_attribute, dev_attr)

#define VPD_ATTR(_name, _mode, _show, _store, _index)   \
    { .dev_attr = __ATTR(_name, _mode, _show, _store),  \
    .index = _index }

#define VPD_DEVICE_ATTR(_name, _mode, _show, _store, _index)    \
    struct vpd_device_attribute vpd_dev_attr_##_name            \
    = VPD_ATTR(_name, _mode, _show, _store, _index)

#define VPD_INFO(fmt, args...) printk( KERN_INFO "[VPD] " fmt, ##args)
#define VPD_WARN(fmt, args...) printk( KERN_WARNING "[VPD] " fmt, ##args)
#define VPD_ERR(fmt, args...)  printk( KERN_ERR  "[VPD] " fmt, ##args)

#ifdef DEBUG_VPD
#    define VPD_DEBUG(fmt, args...)  printk( KERN_DEBUG  "[VPD] " fmt, ##args)
#else
#    define VPD_DEBUG(fmt, args...)
#endif

#endif /* INV_VPD_H */
