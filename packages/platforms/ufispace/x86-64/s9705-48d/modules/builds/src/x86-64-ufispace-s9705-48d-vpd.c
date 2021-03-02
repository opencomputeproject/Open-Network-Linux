#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <linux/dmi.h>
#include <linux/i2c.h>
#include "x86-64-ufispace-s9705-48d-vpd.h"
#include "x86-64-ufispace-s9705-48d-onie-tlvinfo.h"

#ifndef ENABLE_VPD_WRITE
#define ENABLE_VPD_WRITE 0
#endif

static int vpd_major;
static struct class *vpd_class_p = NULL;
static char cEeprom[SYS_EEPROM_MAX_SIZE];
static DEFINE_MUTEX(vpd_mutex); 

static int
__swp_match(struct device *dev,
            const void *data){

    char *name = (char *)data;
    if (strcmp(dev_name(dev), name) == 0)
        return 1;
    return 0;
}

static
int get_vpd_data(struct i2c_client *pi2c_client, int i_offset, char *c_buf)
{
    int iRet;
	
	read_eeprom( pi2c_client, cEeprom);
 	iRet = tlvinfo_decode_tlv(cEeprom, i_offset,c_buf);
    return iRet;
}

#if ENABLE_VPD_WRITE
static
int write_vpd_data(struct i2c_client *pi2c_client, int i_offset, char *c_buf)
{
    int iErr = 0;

    if (read_eeprom(pi2c_client, cEeprom)) {
         printk(KERN_ERR "write vpd data eror at %d-%s\n", __LINE__, __FUNCTION__);
         return -1;
    }

    if (tlvinfo_delete_tlv(cEeprom, i_offset) == TRUE) {
		    }
    if (c_buf) {
        if(!tlvinfo_add_tlv(cEeprom, i_offset , c_buf)) {
            printk(KERN_ERR "write vpd data eror at %d-%s\n", __LINE__, __FUNCTION__);
            iErr = -1;
        } else {
            iErr = prog_eeprom(pi2c_client,cEeprom);
        }
    }
    return iErr;
}
#endif

static struct device *
get_swpdev_by_name(char *name){
    struct device *dev = class_find_device(vpd_class_p,
                                           NULL,
                                           name,
                                           __swp_match);
    return dev;
}

#if ENABLE_VPD_WRITE
static ssize_t
store_attr_vpd(struct device *dev_p,
                      struct device_attribute *attr_p,
                      const char *buf_p,
                      size_t count){
    struct i2c_client *pi2c_client = dev_get_drvdata(dev_p);
	struct vpd_device_attribute *attr = to_vpd_dev_attr(attr_p);
    int iOffset = attr->index;
	int iErr , iLen;
    char *pChar;

    if (!pi2c_client){
        return -ENODEV;
    }
	mutex_lock(&vpd_mutex);

    //-strip 0x0a in the last byte.
    for (iLen = 0, pChar = (char *)buf_p; 
         iLen < 255 && *pChar != 0; 
         iLen++, pChar++) ;
    if (iLen !=0 && *pChar == 0 && *(pChar-1) == 0x0a)
      *(pChar - 1) = 0;
    //-    
   
	iErr = write_vpd_data( pi2c_client, iOffset, (char *)buf_p);

	mutex_unlock(&vpd_mutex);
	return count;
}
#endif

static ssize_t
show_attr_vpd(struct device *dev_p,
                 struct device_attribute *attr_p,
                 char *buf_p){

    struct i2c_client *pi2c_client = dev_get_drvdata(dev_p);
	struct vpd_device_attribute *attr = to_vpd_dev_attr(attr_p);
    int iOffset = attr->index;
	int iErr , iLen;

    if (!pi2c_client){
        return -ENODEV;
    }
	mutex_lock(&vpd_mutex);
	iErr = get_vpd_data( pi2c_client, iOffset, buf_p);
	mutex_unlock(&vpd_mutex);

	if( iErr <= 0 )
		iLen = 0;
    else
	    iLen = snprintf(buf_p, TLV_DECODE_VALUE_MAX_LEN, "%s\n", buf_p);

	return iLen;
}

/* ================= Vpd attribute ========================
 */
#if ENABLE_VPD_WRITE
static VPD_DEVICE_ATTR(product_name     ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_PRODUCT_NAME  );
static VPD_DEVICE_ATTR(part_number      ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_PART_NUMBER );
static VPD_DEVICE_ATTR(serial_number    ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_SERIAL_NUMBER );
static VPD_DEVICE_ATTR(base_mac_address ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_MAC_BASE );
static VPD_DEVICE_ATTR(manufacture_date ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_MANUF_DATE  );
static VPD_DEVICE_ATTR(device_version   ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_DEVICE_VERSION );
static VPD_DEVICE_ATTR(label_revision   ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_LABEL_REVISION );
static VPD_DEVICE_ATTR(platform_name    ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_PLATFORM_NAME );
static VPD_DEVICE_ATTR(onie_version     ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_ONIE_VERSION  );
static VPD_DEVICE_ATTR(mac_addresses    ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_MAC_SIZE  );
static VPD_DEVICE_ATTR(manufacturer     ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_MANUF_NAME  );
static VPD_DEVICE_ATTR(country_code     ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_MANUF_COUNTRY  );
static VPD_DEVICE_ATTR(vendor_name      ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_VENDOR_NAME  );
static VPD_DEVICE_ATTR(diag_version     ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_DIAG_VERSION  );
static VPD_DEVICE_ATTR(service_tag      ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_SERVICE_TAG  );
static VPD_DEVICE_ATTR(vendor_ext       ,S_IWUSR|S_IRUGO, show_attr_vpd, store_attr_vpd, TLV_CODE_VENDOR_EXT  );
static VPD_DEVICE_ATTR(crc32            ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_CRC_32  );
#else
static VPD_DEVICE_ATTR(product_name     ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_PRODUCT_NAME  );
static VPD_DEVICE_ATTR(part_number      ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_PART_NUMBER );
static VPD_DEVICE_ATTR(serial_number    ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_SERIAL_NUMBER );
static VPD_DEVICE_ATTR(base_mac_address ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_MAC_BASE );
static VPD_DEVICE_ATTR(manufacture_date ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_MANUF_DATE  );
static VPD_DEVICE_ATTR(device_version   ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_DEVICE_VERSION );
static VPD_DEVICE_ATTR(label_revision   ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_LABEL_REVISION );
static VPD_DEVICE_ATTR(platform_name    ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_PLATFORM_NAME );
static VPD_DEVICE_ATTR(onie_version     ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_ONIE_VERSION  );
static VPD_DEVICE_ATTR(mac_addresses    ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_MAC_SIZE  );
static VPD_DEVICE_ATTR(manufacturer     ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_MANUF_NAME  );
static VPD_DEVICE_ATTR(country_code     ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_MANUF_COUNTRY  );
static VPD_DEVICE_ATTR(vendor_name      ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_VENDOR_NAME  );
static VPD_DEVICE_ATTR(diag_version     ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_DIAG_VERSION  );
static VPD_DEVICE_ATTR(service_tag      ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_SERVICE_TAG  );
static VPD_DEVICE_ATTR(vendor_ext       ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_VENDOR_EXT  );
static VPD_DEVICE_ATTR(crc32            ,S_IRUGO, show_attr_vpd, NULL, TLV_CODE_CRC_32  );
#endif

static void
clean_vpd_common(void)
{
    dev_t dev_num;
    struct device *device_p;

    device_p = get_swpdev_by_name(VPD_DEVICE);
    if (device_p){
        dev_num = MKDEV(vpd_major, 1);
        device_unregister(device_p);
        device_destroy(vpd_class_p, dev_num);
    }
    VPD_DEBUG("%s: done.\n", __func__);
}


static struct register_attr VpdRegAttr[VPD_ENTRY_SIZE ] ={
    { &vpd_dev_attr_product_name.dev_attr, "vpd_dev_attr_product_name"},
    { &vpd_dev_attr_part_number.dev_attr, "vpd_dev_attr_part_number"},
    { &vpd_dev_attr_serial_number.dev_attr, "vpd_dev_attr_serial_number"},
    { &vpd_dev_attr_base_mac_address.dev_attr, "vpd_dev_attr_base_mac_address"},
    { &vpd_dev_attr_manufacture_date.dev_attr, "vpd_dev_attr_manufacture_date"},
    { &vpd_dev_attr_device_version.dev_attr, "vpd_dev_attr_device_version"},
    { &vpd_dev_attr_label_revision.dev_attr, "vpd_dev_attr_label_revision"},
    { &vpd_dev_attr_platform_name.dev_attr, "vpd_dev_attr_platform_name"},
    { &vpd_dev_attr_onie_version.dev_attr, "vpd_dev_attr_onie_version"},
    { &vpd_dev_attr_mac_addresses.dev_attr, "vpd_dev_attr_mac_addresses"},
    { &vpd_dev_attr_manufacturer.dev_attr, "vpd_dev_attr_manufacturer"},
    { &vpd_dev_attr_country_code.dev_attr, "vpd_dev_attr_country_code"},
    { &vpd_dev_attr_vendor_name.dev_attr, "vpd_dev_attr_vendor_name"},
    { &vpd_dev_attr_diag_version.dev_attr, "vpd_dev_attr_diag_version"},
    { &vpd_dev_attr_service_tag.dev_attr, "vpd_dev_attr_service_tag"},
    { &vpd_dev_attr_vendor_ext.dev_attr, "vpd_dev_attr_vendor_ext"},
    { &vpd_dev_attr_crc32.dev_attr, "vpd_dev_attr_crc32"},
};

static int
register_vpd_attr(struct device *device_p){

    char *err_attr = NULL;
    int i;

    for( i = 0 ; i <VPD_ENTRY_SIZE ;i++ ){
        if(device_create_file(device_p, VpdRegAttr[ i ].attr) < 0) {
            err_attr = VpdRegAttr[ i ].errmsg;
            goto err_register_vpd_attr;
        }
    }
    return 0;

err_register_vpd_attr:
    VPD_ERR("%s: %s\n", __func__, err_attr);
    return -1;
}


static int
register_vpd_device(void)
{
    struct device *device_p = NULL;
    int minor_comm = 0; /* Default minor number for common device */
    dev_t dev_num  = MKDEV(vpd_major, minor_comm);
    char *err_msg  = "ERROR";
    struct i2c_adapter *adap;
    struct i2c_client *vpd_i2c_client = NULL;

    vpd_i2c_client = kzalloc(sizeof(*vpd_i2c_client), GFP_KERNEL);
    if (!vpd_i2c_client){
        printk(KERN_ERR "can not kzalloc client:%d", VPD_I2C_BUS);
        goto err_register_vpd_device;
    }

    adap = i2c_get_adapter(VPD_I2C_BUS);
    vpd_i2c_client->adapter = adap;
    vpd_i2c_client->addr = VPD_I2C_ADDR;

    device_p = device_create(vpd_class_p,     /* struct class *cls     */
                             NULL,            /* struct device *parent */
                             dev_num,         /* dev_t devt            */
                             vpd_i2c_client,  /* void *private_data    */
                             VPD_DEVICE);    /* const char *fmt       */
    if (IS_ERR(device_p)){
        err_msg = "device_create fail";
        goto err_register_vpd_device_1;
    }
    if (register_vpd_attr(device_p) < 0) {
        err_msg = "register_vpd_attr fail";
        goto err_register_vpd_device_2;
    }
    return 0;

err_register_vpd_device_2:
    device_unregister(device_p);
    device_destroy(vpd_class_p, dev_num);
err_register_vpd_device_1:
    kfree(vpd_i2c_client);
    vpd_i2c_client = NULL;
err_register_vpd_device:
    VPD_ERR("%s: %s\n", __func__, err_msg);
    return -1;
}

static int
register_vpd_module(void)
{
    dev_t vpd_devt  = 0;

    if (alloc_chrdev_region(&vpd_devt, 0, 1, VPD_DEVICE) < 0){
        VPD_WARN("Allocate VPD MAJOR failure! \n");
        goto err_register_vpd_module;
    }
    vpd_major  = MAJOR(vpd_devt);

    /* Create class object */
    vpd_class_p = class_create(THIS_MODULE, EEPROM_CLASS);
    if (IS_ERR(vpd_class_p)) {
        VPD_ERR("Create class failure! \n");
        goto err_register_vpd_module_1;
    }
    return 0;

err_register_vpd_module_1:
    unregister_chrdev_region(MKDEV(vpd_major, 0), 1);
err_register_vpd_module:
    return -1;
}


static int
init_vpd_common(void)
{
    char *err_msg = "ERR";
    
    if (register_vpd_device() < 0) {
        err_msg = "register_vpd_device fail";
        goto err_init_vpd_common;
    }
    return 0;

err_init_vpd_common:
    VPD_ERR("%s: %s\n", __func__, err_msg);
    return -1;
}


static int __init
vpd_module_init(void)
{
    if (register_vpd_module() < 0){
        goto err_vpd_module_init;
    }
    if (init_vpd_common() < 0){
        goto err_vpd_module_init_1;
    }
    VPD_INFO("UfiSpace vpd module V.%s initial success.\n", VPD_VERSION);
    return 0;

err_vpd_module_init_1:
    class_unregister(vpd_class_p);
    class_destroy(vpd_class_p);
    unregister_chrdev_region(MKDEV(vpd_major, 0), 1);
err_vpd_module_init:
    VPD_ERR("UfiSpace vpd module V.%s initial failure.\n", VPD_VERSION);
    return -1;
}


static void __exit
vpd_module_exit(void)
{
    clean_vpd_common();
    class_unregister(vpd_class_p);
    class_destroy(vpd_class_p);
    unregister_chrdev_region(MKDEV(vpd_major, 0), 1);
    VPD_INFO("Remove UfiSpace vpd module success.\n");
}


/*  Module information  */
MODULE_AUTHOR(VPD_AUTHOR);
MODULE_DESCRIPTION(VPD_DESC);
MODULE_VERSION(VPD_VERSION);
MODULE_LICENSE(VPD_LICENSE);

module_init(vpd_module_init);
module_exit(vpd_module_exit);
