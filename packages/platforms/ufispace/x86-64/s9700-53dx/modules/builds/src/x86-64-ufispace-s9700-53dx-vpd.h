#ifndef UFI_VPD_H
#define UFI_VPD_H

#define EEPROM_CLASS   "eeprom"
#define VPD_DEVICE     "x86_64_ufispace_s9700_53dx_onie_syseeprom"
#define VPD_AUTHOR     "UfiSpace <feng.cf.lee@ufispace.com>"
#define VPD_DESC       "UfiSpace eeprom vpd driver"
#define VPD_VERSION    "1.0.0"
#define VPD_LICENSE    "GPL"

#define VPD_ENTRY_SIZE        (17)
#define VPD_I2C_BUS           (0)
#define VPD_I2C_ADDR          (0x57)

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

#endif /* UFI_VPD_H */
