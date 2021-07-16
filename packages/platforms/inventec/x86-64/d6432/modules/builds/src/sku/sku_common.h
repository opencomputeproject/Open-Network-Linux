#ifndef __SKU_COMMON
#define __SKU_COMMON

#include "../inv_swps.h"

#define SKU_LOG_ERR(fmt, args...) \
    do { \
        if (logLevel & SKU_ERR_LEV) \
        { \
            printk (KERN_ERR "[SKU]%s:"fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

#define SKU_LOG_INFO(fmt, args...) \
    do { \
        if (logLevel & SKU_INFO_LEV) \
        { \
            printk (KERN_INFO "[SKU]%s:"fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

#define SKU_LOG_DBG(fmt, args...) \
    do { \
        if (logLevel & SKU_DBG_LEV) \
        { \
            printk (KERN_INFO "[SKU]%s:"fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

struct inv_i2c_client_t {
    struct i2c_client *client;
    struct mutex lock;
};

struct device *find_dev( const char *name, struct bus_type *bus);
int gpio_base_find(int *gpio_base);  
#endif /*__SKU_COMMON*/
