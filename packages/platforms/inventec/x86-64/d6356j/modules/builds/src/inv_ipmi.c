/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>

//=================================
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/ipmi.h>
#include <linux/ipmi_smi.h>
#include <linux/hwmon-sysfs.h>


#define E_NONE 0


#define IPMI_MAX_INTF            (4)

#define TLV_AREA_OFFSETS_IDX_START      1
#define TLV_PRODUCT_INFO_OFFSET_IDX     5
#define TLV_PRODUCT_INFO_AREA_START     3
#define TLV_ATTR_TYPE_SERIAL            5
#define TLV_ATTR_TYPE_MODEL             2

#define	LEN_BMC_WDT_RESET         0
#define	LEN_BMC_WDT_RESET_RESP    8
#define	LEN_BMC_WDT_SET           6
#define	LEN_BMC_WDT_SET_RESP      1
#define	LEN_BMC_WDT_GET           0
#define	LEN_BMC_WDT_GET_RESP      8
#define LEN_FRU_READ_RESP         4

#define	BMC_WDT_INFO_TIMER_USE_TIMER_USE_MASK           0x7
#define	BMC_WDT_INFO_TIMER_USE_STARTED_MASK             0x40
#define	BMC_WDT_INFO_TIMER_ACT_TIMEOUT_ACT_MASK         0x7
#define	BMC_WDT_INFO_TIMER_ACT_PRETIMEOUT_INTRP_MASK    0x70

#define	BMC_WDT_INFO_TIMER_USE_TIMER_USE_OFFSET         0
#define	BMC_WDT_INFO_TIMER_USE_STARTED_OFFSET           6
#define	BMC_WDT_INFO_TIMER_ACT_TIMEOUT_ACT_OFFSET       0
#define	BMC_WDT_INFO_TIMER_ACT_PRETIMEOUT_INTRP_OFFSET  4

enum {
	NETFN_APP = 0x06,
	NETFN_STORAGE=0x0a,
	NETFN_OEM = 0x30
} IPMI_NetFn;

enum {
	/* FRU*/
	CMD_FRU_READDATA  = 0x11,
	/* BMC watchdog */
	CMD_BMC_WDT_RESET = 0x22,
	CMD_BMC_WDT_SET   = 0x24,
	CMD_BMC_WDT_GET   = 0x25,
	/* OEM */
	CMD_OEM_GETDATA   = 0x31,
	CMD_OEM_SETDATA   = 0x32
} IPMI_Cmd;


/* Reference IPMI spec V2.0 chap 27.7*/
enum {
	BMC_WDT_INFO_TIMER_USE,
	BMC_WDT_INFO_TIMER_ACTIONS,
	BMC_WDT_INFO_PRE_TIMEOUT_INTERVAL,
	BMC_WDT_INFO_TIMER_USE_EXPIR_FLAG,
	BMC_WDT_INFO_INITIAL_COUNTDOWN_LSB,
	BMC_WDT_INFO_INITIAL_COUNTDOWN_MSB,
	BMC_WDT_INFO_PRESENT_COUNTDOWN_LSB,
	BMC_WDT_INFO_PRESENT_COUNTDOWN_MSB
} IPMI_BMC_WDT_Info_Type;


#define BMC_WDT_MAX_COUNTDOWN_VALUE        (0xFF*100)
#define BMC_WDT_RESET_RETRY_COUNT          1

#define MAX_IPMI_RECV_LENGTH 0xff

static struct kset *inv_ipmi_kset;

struct ipmi_result {
	char result[MAX_IPMI_RECV_LENGTH];
	int result_length;
};
static struct ipmi_result ipmiresult;
static ipmi_user_t ipmi_user = NULL;
static struct ipmi_user_hndl ipmi_handlers;

static atomic_t dummy_count = ATOMIC_INIT(0);
static struct ipmi_smi_msg halt_smi_msg;
static struct ipmi_recv_msg halt_recv_msg;

DEFINE_MUTEX(ipmi_mutex);
DEFINE_MUTEX(ipmi2_mutex);


/*
------------------ ipmi relate functions ------------------
*/
static void dummy_smi_free(struct ipmi_smi_msg *msg)
{
	atomic_dec(&dummy_count);
}
static void dummy_recv_free(struct ipmi_recv_msg *msg)
{
	atomic_dec(&dummy_count);
}

static void msg_handler(struct ipmi_recv_msg *recv_msg, void* handler_data)
{
	struct ipmi_result *msg_result = recv_msg->user_msg_data;

	if (recv_msg->msg.data[0] == 0 && recv_msg->msg.data_len > 0) {
		msg_result->result_length = recv_msg->msg.data_len - 1;
		memcpy(msg_result->result, &recv_msg->msg.data[1], recv_msg->msg.data_len - 1);
	}
	ipmi_free_recv_msg(recv_msg);
	mutex_unlock(&ipmi_mutex);

	return;
}


int inv_ipmi_command(char NetFn, char cmd, char *data, int data_length, char* result, size_t result_max, int* result_length)
{
	int ret = E_NONE;
	int i;
	int timeout;

	//wait previous command finish at least 50msec
	timeout = 50;
	while ((mutex_is_locked(&ipmi_mutex) == true
	        || (mutex_is_locked(&ipmi2_mutex) == true))
	        && (--timeout) > 0) {
		usleep_range(1000, 1010);
	}
	if (timeout == 0) {
		return -EBUSY;
	}
	mutex_lock(&ipmi_mutex);
	mutex_lock(&ipmi2_mutex);

	if (ipmi_user == NULL) {
		for (i = 0, ret = 1; i < IPMI_MAX_INTF && ret; i++) {
			ret = ipmi_create_user(i, &ipmi_handlers, NULL, &ipmi_user);
		}
	}

	if (ret < 0) {
		mutex_unlock(&ipmi_mutex);
		mutex_unlock(&ipmi2_mutex);
		return ret;
	} else {
		struct	ipmi_system_interface_addr addr;
		struct	kernel_ipmi_msg msg;
		uint8_t msg_data[data_length];

		addr.addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
		addr.channel = IPMI_BMC_CHANNEL;
		addr.lun = 0;

		memset(&msg, 0, sizeof(struct kernel_ipmi_msg));
		msg.netfn = NetFn;
		msg.cmd = cmd;
		if (data != NULL) {
			memcpy(msg_data, data, data_length);
			msg.data = msg_data;
			msg.data_len = data_length;
		}

		ret = ipmi_request_supply_msgs(ipmi_user,
		                              (struct ipmi_addr*)&addr, 0,
		                              &msg, &ipmiresult, &halt_smi_msg, &halt_recv_msg, 0);
		if (ret) {
			mutex_unlock(&ipmi_mutex);
			mutex_unlock(&ipmi2_mutex);
			return -ENXIO;
		}

		//skip command if 1sec no response from remote
		timeout = 1000;
		while (mutex_is_locked(&ipmi_mutex) == 1 && (--timeout) > 0) {
			usleep_range(1000, 1100);
		}
		if (timeout == 0) {
			mutex_unlock(&ipmi2_mutex);
			return -EPERM;
		} else {
			*result_length = ipmiresult.result_length;
			if ( *result_length > result_max) {
				memcpy(result, ipmiresult.result, result_max);
			} else {
				memcpy(result, ipmiresult.result, *result_length);
			}
			mutex_unlock(&ipmi2_mutex);
			return ret;
		}
	}
	return ret;
}

static int inv_ipmi_wdt_reset(u8 *resp, int *resp_len)
{
	int ret = E_NONE;

	ret = inv_ipmi_command(NETFN_APP, CMD_BMC_WDT_RESET, NULL, 0, resp, LEN_BMC_WDT_RESET_RESP, resp_len);
	return ret;
}

static int inv_ipmi_wdt_set(u8 *buf, size_t buf_len)
{
	u8 resp[LEN_BMC_WDT_SET_RESP];
	int resp_len;
	int ret = E_NONE;

	ret = inv_ipmi_command(NETFN_APP, CMD_BMC_WDT_SET, buf, buf_len, resp, LEN_BMC_WDT_SET_RESP, &resp_len);

	return ret;
}


static int inv_ipmi_wdt_get(u8 *buf, size_t buf_len)
{
	int ret = E_NONE;
	uint resp_len;

	ret = inv_ipmi_command(NETFN_APP, CMD_BMC_WDT_GET, NULL, 0, buf, buf_len, &resp_len);

	/* If resp len is not equal to wdt info len, return error */
	if (resp_len != LEN_BMC_WDT_GET_RESP) {
		ret = -EPERM;
	}

	return ret;
}

#if 0
static int inv_ipmi_oem_read(u8 *buf, u8 offset, size_t count)
{
	uint8_t data[2];
	int result_len = 0;
	int ret;

	data[0] = offset;
	data[1] = count;

	ret = inv_ipmi_command(NETFN_OEM, CMD_OEM_GETDATA, data, 2, buf, count, &result_len);

	if (ret) {
		printk("[INV_IPMI]%s: ret %d\n", __func__, ret);
	}

	return result_len;
}

static int inv_ipmi_oem_write(char *buf, unsigned offset, size_t count)
{
	uint8_t data[count + 1], result[1];
	int result_len;
	int ret = E_NONE;

	data[0] = offset;
	memcpy(&data[1], buf, count);

	ret = inv_ipmi_command(NETFN_OEM, CMD_OEM_SETDATA, data, count + 1, result, sizeof(result), &result_len);
	if (ret) {
		printk("[INV_IPMI]%s: ret %d\n", __func__, ret);
	}
	return count;
}
#endif

static int _inv_ipmi_init(void)
{
	int ret = E_NONE;

	ipmi_handlers.ipmi_recv_hndl = msg_handler;

	halt_smi_msg.done = dummy_smi_free;
	halt_recv_msg.done = dummy_recv_free;

	return ret;
}

/*
------------------ watchdog start ------------------
*/
static ssize_t show_wdt_start(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                char *buf)
{
	int ret = 0;
	bool start;
	u8 wdt_info[LEN_BMC_WDT_GET_RESP];

	ret = inv_ipmi_wdt_get(wdt_info, LEN_BMC_WDT_GET_RESP);

	if (ret == E_NONE) {
		start =
		    (wdt_info[BMC_WDT_INFO_TIMER_USE] & BMC_WDT_INFO_TIMER_USE_STARTED_MASK)
		    << BMC_WDT_INFO_TIMER_USE_STARTED_OFFSET;
		snprintf(buf, PAGE_SIZE, "%d\n", start);
	}

	if(ret != E_NONE) {
		snprintf(buf, PAGE_SIZE, "Error %d\n", ret);
	}

	return strlen(buf);
}

static ssize_t set_wdt_start(struct kobject *kobj, struct kobj_attribute *attr,
        const char* _buf, size_t count)
{
	int ret = E_NONE;
	int try_count;
	uint resp_len;
	int start;
	int is_started;
	u8 wdt_info[LEN_BMC_WDT_GET_RESP];
	u8 resp[LEN_BMC_WDT_RESET_RESP];


	ret = kstrtol(_buf, 10, (long*)&start);

	if ((start != true)&&(start != false)) {
		ret = -EPERM;
	}

	if (ret == E_NONE) {
		ret = inv_ipmi_wdt_get(wdt_info, LEN_BMC_WDT_GET_RESP);
		if (ret == E_NONE) {
			is_started =
			    (wdt_info[BMC_WDT_INFO_TIMER_USE] & BMC_WDT_INFO_TIMER_USE_STARTED_MASK)
			    << BMC_WDT_INFO_TIMER_USE_STARTED_OFFSET;
		}
	}

	if (ret == E_NONE) {
		if(start) {
			/* No metter timer is started or not, just reset*/
			/* Set count to original try and retry count*/
			try_count = BMC_WDT_RESET_RETRY_COUNT+1;
			while(try_count) {
				ret = inv_ipmi_wdt_reset(resp, &resp_len);
				if (ret == E_NONE) {
					if(resp_len > 0) {
						/* 
							Abnormal case, may met BMC reset case.
							When BMC reset, WDT should set again.
							So the command set success, but response hase error.
						*/
						try_count--;
						if(try_count == 0){
							ret = -EPERM;
						} else {
							ret = inv_ipmi_wdt_set(wdt_info, LEN_BMC_WDT_SET);
							if(ret != E_NONE) {
								/* WDT set fail, abort*/
								try_count = 0;
							}
						}
					} else {
						/* No response, means success*/
						try_count = 0;
					}
				} else {
					/* reset fail directly*/
					try_count = 0;
				}
			}
		} else {
			/* If timer is not started, do nothing*/
			if(is_started) {
				wdt_info[BMC_WDT_INFO_TIMER_USE] &= (~BMC_WDT_INFO_TIMER_USE_STARTED_MASK);
				ret = inv_ipmi_wdt_set(wdt_info, LEN_BMC_WDT_SET);
			}
		}
	}

	if(ret != E_NONE) {
		printk("[INV_IPMI]%s:ret %d\n", __func__, ret);
	}

	return count;
}

static struct kobj_attribute wdt_started_att =
    __ATTR(wdt_start, S_IRUGO | S_IWUSR, show_wdt_start, set_wdt_start);

/*
------------------ watchdog initial_countdown ------------------
*/
static ssize_t show_wdt_initial_countdown(struct kobject *kobj,
        struct kobj_attribute *attr,
        char *buf)
{
	int ret = 0;
	int initial_countdown;
	u8 wdt_info[LEN_BMC_WDT_GET_RESP];

	ret = inv_ipmi_wdt_get(wdt_info, LEN_BMC_WDT_GET_RESP);

	if (ret == E_NONE) {
		initial_countdown = 100 *
		                    ((wdt_info[BMC_WDT_INFO_INITIAL_COUNTDOWN_MSB] << 8) +
		                     (wdt_info[BMC_WDT_INFO_INITIAL_COUNTDOWN_LSB]));
		snprintf(buf, PAGE_SIZE, "%d ms\n", initial_countdown);
	}

	if(ret != E_NONE) {
		snprintf(buf, PAGE_SIZE, "Error %d\n", ret);
	}

	return strlen(buf);
}

static ssize_t set_wdt_initial_countdown(struct kobject *kobj, struct kobj_attribute *attr,
        const char* _buf, size_t count)
{
	int ret = E_NONE;
	int initial_countdown;
	u8 wdt_info[LEN_BMC_WDT_GET_RESP];

	ret = kstrtol(_buf, 10, (long*)&initial_countdown);

	if ((initial_countdown < 0)&&(initial_countdown > BMC_WDT_MAX_COUNTDOWN_VALUE)) {
		ret = -EPERM;
	}

	if (ret == E_NONE) {
		ret = inv_ipmi_wdt_get(wdt_info, LEN_BMC_WDT_GET_RESP);
		if (ret == E_NONE) {
			/*initial_countdown is in ms, in spec would use 100ms*/
			wdt_info[BMC_WDT_INFO_INITIAL_COUNTDOWN_LSB] = ((initial_countdown / 100) & 0xFF);
			wdt_info[BMC_WDT_INFO_INITIAL_COUNTDOWN_MSB] = (((initial_countdown / 100) >> 8) & 0xFF);

			ret = inv_ipmi_wdt_set(wdt_info, LEN_BMC_WDT_SET);
		}
	}

	if (ret != E_NONE) {
		printk("[INV_IPMI]%s:ret %d\n", __func__, ret);
	}

	return count;
}


static struct kobj_attribute wdt_initial_countdown_att =
    __ATTR(wdt_initial_countdown, S_IRUGO | S_IWUSR, show_wdt_initial_countdown, set_wdt_initial_countdown);


/*
------------------ watchdog present_countdown ------------------
*/
static ssize_t show_wdt_present_countdown(struct kobject *kobj,
        struct kobj_attribute *attr,
        char *buf)
{
	int ret = 0;
	int present_countdown;
	u8 wdt_info[LEN_BMC_WDT_GET_RESP];

	ret = inv_ipmi_wdt_get(wdt_info, LEN_BMC_WDT_GET_RESP);

	if (ret == E_NONE) {
		present_countdown = 100 *
		                    ((wdt_info[BMC_WDT_INFO_PRESENT_COUNTDOWN_MSB] << 8) +
		                     (wdt_info[BMC_WDT_INFO_PRESENT_COUNTDOWN_LSB]));
		snprintf(buf, PAGE_SIZE, "%d ms\n", present_countdown);
	}

	if(ret != E_NONE) {
		snprintf(buf, PAGE_SIZE, "Error %d\n", ret);
	}

	return strlen(buf);
}


static struct kobj_attribute wdt_present_countdown_att =
    __ATTR(wdt_present_countdown, S_IRUGO, show_wdt_present_countdown, NULL);

/*
------------------ fru data ------------------
*/
static ssize_t show_fru_data(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int ret;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	uint8_t data[LEN_FRU_READ_RESP], result[MAX_IPMI_RECV_LENGTH];
        int result_len = 0, attr_length=0/*, attr_type=TLV_ATTR_TYPE_SERIAL, str_offset=0*/;
	int i;
	int offset = 1;  /* first bit in ipmi raw info dump is the size of result, then the fru data */
	uint8_t data_size;
	char temp[MAX_IPMI_RECV_LENGTH] ={0};
	int fru_index=attr->index;

	data[0] = fru_index;
	data[1] = 0;
	data[2] = 0;
	data[3] = MAX_IPMI_RECV_LENGTH;

	ret = inv_ipmi_command(NETFN_STORAGE, CMD_FRU_READDATA, data ,LEN_FRU_READ_RESP, result, MAX_IPMI_RECV_LENGTH, &result_len);
	if (ret == E_NONE) {
		data_size=result[0];
		offset+=result[TLV_PRODUCT_INFO_OFFSET_IDX]*8; //spec defined: offset are in multiples of 8 bytes
		offset+=TLV_PRODUCT_INFO_AREA_START; 
		
	    for(i=1; i<=TLV_ATTR_TYPE_SERIAL; i++) {
	        if(offset>data_size){
                /* Due to return -1 will damaged ONLP apps, Neil@21/06/10 */
                snprintf(buf, 24, "FRU data not ready\n");
	            return strlen(buf);
	        }
	        attr_length=result[offset]&(0x3F);    //spec defined: length are set in last 6 bits
	        if(i==TLV_ATTR_TYPE_MODEL){
	        	memcpy(temp,result+offset+1,attr_length*sizeof(char));
	        	snprintf(buf,MAX_IPMI_RECV_LENGTH,"Model:%s\n",temp);
	        }else if(i==TLV_ATTR_TYPE_SERIAL){
	        	memcpy(temp,result+offset+1,attr_length*sizeof(char));
	        	snprintf(buf,MAX_IPMI_RECV_LENGTH,"%sSerial:%s\n",buf,temp);
	        }
	        offset+=(attr_length+1);
	    }
	}
	return strlen(buf);
}


static SENSOR_DEVICE_ATTR(psu1_fru, S_IRUGO, show_fru_data, 0, 1);
static SENSOR_DEVICE_ATTR(psu2_fru, S_IRUGO, show_fru_data, 0, 2);
static SENSOR_DEVICE_ATTR(fan1_fru, S_IRUGO, show_fru_data, 0, 3);
static SENSOR_DEVICE_ATTR(fan2_fru, S_IRUGO, show_fru_data, 0, 4);
static SENSOR_DEVICE_ATTR(fan3_fru, S_IRUGO, show_fru_data, 0, 5);
static SENSOR_DEVICE_ATTR(fan4_fru, S_IRUGO, show_fru_data, 0, 6);
//static struct kobj_attribute show_fru_data_attr=
	//__ATTR(fan3_serial, S_IRUGO, show_fan_serial, NULL);

/*
-------- declare attribute
*/
static struct attribute *inv_ipmi_attrs[] = {
	&wdt_started_att.attr,
	&wdt_initial_countdown_att.attr,
	&wdt_present_countdown_att.attr,
	&sensor_dev_attr_psu1_fru.dev_attr.attr,
	&sensor_dev_attr_psu2_fru.dev_attr.attr,
	&sensor_dev_attr_fan1_fru.dev_attr.attr,
	&sensor_dev_attr_fan2_fru.dev_attr.attr,
	&sensor_dev_attr_fan3_fru.dev_attr.attr,
	&sensor_dev_attr_fan4_fru.dev_attr.attr,
	NULL,
};

static const struct attribute_group inv_ipmi_group = {
	.attrs = inv_ipmi_attrs,
};

static int __init inv_ipmi_init(void)
{
	int ret = E_NONE;

	inv_ipmi_kset = kset_create_and_add("inv_ipmi", NULL, NULL);
	if (!inv_ipmi_kset) {
		ret = -ENOMEM;
	}

	if (ret == E_NONE) {
		ret = sysfs_create_group(&inv_ipmi_kset->kobj, &inv_ipmi_group);
		if (ret) {
			kset_unregister(inv_ipmi_kset);
			ret = -ENOSYS;
		}
	}

	if (ret == E_NONE) {
		ret = _inv_ipmi_init();
		if (ret) {
			kset_unregister(inv_ipmi_kset);
		}
	}

	if (ret != E_NONE) {
		printk("[INV_IPMI]%s:ret %d\n", __func__, ret);
	}
	return ret;
}

static void __exit inv_ipmi_exit(void)
{
	if (ipmi_user != NULL) {
		ipmi_destroy_user(ipmi_user);
	}
	sysfs_remove_group(&inv_ipmi_kset->kobj, &inv_ipmi_group);
	kset_unregister(inv_ipmi_kset);
}

module_init(inv_ipmi_init);
module_exit(inv_ipmi_exit);

MODULE_AUTHOR("Ting.Jack <ting.jack@inventec>");
MODULE_DESCRIPTION("Inventec ipmi driver");
MODULE_LICENSE("GPL");

