#include <linux/i2c.h>
#include <linux/version.h>
#include <linux/i2c-gpio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,151)
#include <linux/platform_data/pca954x.h>
#else
#include <linux/i2c/pca954x.h>
#endif

struct inv_i2c_board_info {
    int ch;
    int size;
    struct i2c_board_info *board_info;
    int probe;
};

#define bus_id(id)  (id)
static struct pca954x_platform_mode pca9641_modes_1[] = {
    {.adap_id = bus_id(2),},
};
static struct pca954x_platform_mode pca9641_modes_2[] = {
    {.adap_id = bus_id(3),},
};
static struct pca954x_platform_mode mux_modes_0[] = {
    {.adap_id = bus_id(6),},    {.adap_id = bus_id(7),},
    {.adap_id = bus_id(8),},    {.adap_id = bus_id(9),},
    {.adap_id = bus_id(10),},    {.adap_id = bus_id(11),},
    {.adap_id = bus_id(12),},    {.adap_id = bus_id(13),},
};
static struct pca954x_platform_mode mux_modes_0_0[] = {
    {.adap_id = bus_id(14),},    {.adap_id = bus_id(15),},
    {.adap_id = bus_id(16),},    {.adap_id = bus_id(17),},
    {.adap_id = bus_id(18),},    {.adap_id = bus_id(19),},
    {.adap_id = bus_id(20),},    {.adap_id = bus_id(21),},
};

static struct pca954x_platform_mode mux_modes_0_1[] = {
    {.adap_id = bus_id(22),},    {.adap_id = bus_id(23),},
    {.adap_id = bus_id(24),},    {.adap_id = bus_id(25),},
    {.adap_id = bus_id(26),},    {.adap_id = bus_id(27),},
    {.adap_id = bus_id(28),},    {.adap_id = bus_id(29),},
};

static struct pca954x_platform_mode mux_modes_0_2[] = {
    {.adap_id = bus_id(30),},    {.adap_id = bus_id(31),},
    {.adap_id = bus_id(32),},    {.adap_id = bus_id(33),},
    {.adap_id = bus_id(34),},    {.adap_id = bus_id(35),},
    {.adap_id = bus_id(36),},    {.adap_id = bus_id(37),},
};
static struct pca954x_platform_mode mux_modes_0_3[] = {
    {.adap_id = bus_id(38),},    {.adap_id = bus_id(39),},
    {.adap_id = bus_id(40),},    {.adap_id = bus_id(41),},
    {.adap_id = bus_id(42),},    {.adap_id = bus_id(43),},
    {.adap_id = bus_id(44),},    {.adap_id = bus_id(45),},
};

static struct pca954x_platform_data pca9641_data_1 = {
    .modes          = pca9641_modes_1,
    .num_modes      = 1,
};
static struct pca954x_platform_data pca9641_data_2 = {
    .modes          = pca9641_modes_2,
    .num_modes      = 1,
};
static struct pca954x_platform_data mux_data_0 = {
    .modes          = mux_modes_0,
    .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_0 = {
    .modes          = mux_modes_0_0,
    .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_1 = {
    .modes          = mux_modes_0_1,
    .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_2 = {
    .modes          = mux_modes_0_2,
    .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_3 = {
    .modes          = mux_modes_0_3,
    .num_modes      = 8,
};

static struct i2c_board_info i2c_device_info0[] __initdata = {
    {"pca9641",    0, 0x76, &pca9641_data_1, 0, 0},    //PCA9641-1
};
static struct i2c_board_info i2c_device_info2[] __initdata = {
    {"inv_cpld",   0, 0x77, 0, 0, 0},                  //CPLD
};
static struct i2c_board_info i2c_device_info4[] __initdata = {
    {"inv_psu",    0, 0x58, 0, 0, 0},                  //PSU1
    {"inv_psu",    0, 0x59, 0, 0, 0},                  //PSU2
    {"inv_psu",      0, 0x5a, 0, 0, 0},                  //PSU1
    {"inv_psu",      0, 0x5b, 0, 0, 0},                  //PSU2
};
static struct i2c_board_info i2c_device_info5[] __initdata = {
    {"inv_ucd90160",     0, 0x34, 0, 0, 0},            //switch board ucd90160
    {"inv_ucd90160",     0, 0x6b, 0, 0, 0},            //CPU board ucd90160
};
static struct i2c_board_info i2c_device_info1[] __initdata = {
    {"pca9548",          0, 0x70, &mux_data_0, 0, 0},    //mux root
};
static struct i2c_board_info i2c_device_info6[] __initdata = {
    {"pca9548",          0, 0x72, &mux_data_0_0, 0, 0},
};
static struct i2c_board_info i2c_device_info7[] __initdata = {
    {"pca9548",          0, 0x72, &mux_data_0_1, 0, 0},
};
static struct i2c_board_info i2c_device_info8[] __initdata = {
    {"pca9548",          0, 0x72, &mux_data_0_2, 0, 0},
};
static struct i2c_board_info i2c_device_info9[] __initdata = {
    {"pca9548",          0, 0x72, &mux_data_0_3, 0, 0},
};
static struct i2c_board_info i2c_device_info11[] __initdata = {
    {"pca9641",          0, 0x73, &pca9641_data_2, 0, 0},   //PCA9641-2
};
static struct i2c_board_info i2c_device_info3[] __initdata = {
    {"max6695",          0, 0x18, 0, 0, 0},                 //ASIC Temp
    {"tmp75",            0, 0x48, 0, 0, 0},                 //CPU Board Temp
    {"tmp75",            0, 0x49, 0, 0, 0},                 //Fan Board Temp
    {"tmp75",            0, 0x4A, 0, 0, 0},                 //Temp
    {"tmp75",            0, 0x4D, 0, 0, 0},                 //Temp
    {"tmp75",            0, 0x4E, 0, 0, 0},                 //Temp
};
#define INV_PLATFORM_CLIENT_MAX_NUM 100 /*A big enough number for sum of i2cdev_list[i].size */
static int client_list_index = 0;
static struct i2c_client *client_list[INV_PLATFORM_CLIENT_MAX_NUM] = {0};

static struct inv_i2c_board_info i2cdev_list[] = {
    {bus_id(0), ARRAY_SIZE(i2c_device_info0),  i2c_device_info0, 0},  //PCA9641-1
    {bus_id(0), ARRAY_SIZE(i2c_device_info1),  i2c_device_info1, 1},  //mux root
    {bus_id(1), ARRAY_SIZE(i2c_device_info1),  i2c_device_info1, 1},  //mux root
    {bus_id(6), ARRAY_SIZE(i2c_device_info6),  i2c_device_info6, 0},  //mux CH0
    {bus_id(7), ARRAY_SIZE(i2c_device_info7),  i2c_device_info7, 0},  //mux CH1
    {bus_id(8), ARRAY_SIZE(i2c_device_info8),  i2c_device_info8, 0},  //mux CH2
    {bus_id(9), ARRAY_SIZE(i2c_device_info9),  i2c_device_info9, 0},  //mux CH3
    {bus_id(11),ARRAY_SIZE(i2c_device_info11), i2c_device_info11,0},  //PCA9641-2
    {bus_id(2), ARRAY_SIZE(i2c_device_info2),  i2c_device_info2, 0},  //CPLD
    {bus_id(3), ARRAY_SIZE(i2c_device_info3),  i2c_device_info3, 0},  //Temperature
    {bus_id(2), ARRAY_SIZE(i2c_device_info4),  i2c_device_info4, 1},  //PSU1 and PSU2
    {bus_id(2), ARRAY_SIZE(i2c_device_info5),  i2c_device_info5, 0},  //UCD90160
};

static struct   platform_device         *device_i2c_gpio0;
static struct   i2c_gpio_platform_data  i2c_gpio_platdata0 = {
    .scl_pin = 58,
    .sda_pin = 75,
    
    .udelay  = 5, //5:100kHz
    .sda_is_open_drain = 0,
    .scl_is_open_drain = 0,
    .scl_is_output_only = 0
};

static int __init inv_platform_init(void)
{
    struct i2c_adapter *adap = NULL;
    struct i2c_client *e = NULL;
    int ret = 0;
    int i,j,k;

    device_i2c_gpio0 = platform_device_alloc("i2c-gpio", 1);
    if (!device_i2c_gpio0) {
        printk(KERN_ERR "i2c-gpio: platform_device_alloc fail\n");
        return -ENOMEM;
    }
    device_i2c_gpio0->name = "i2c-gpio";
    device_i2c_gpio0->id     = 1;
    device_i2c_gpio0->dev.platform_data = &i2c_gpio_platdata0;

    ret = platform_device_add(device_i2c_gpio0);
    if (ret) {
        printk(KERN_ERR "i2c-gpio: platform_device_add fail %d\n", ret);
    }
    msleep(10);

    for(i=0; i<ARRAY_SIZE(i2cdev_list); i++) {
        adap = i2c_get_adapter( i2cdev_list[i].ch );
        if (adap == NULL) {
            printk("platform get channel %d adapter fail\n", i);
            continue;
        }
        for(j=0; j<i2cdev_list[i].size; j++) {
            for(k=0; k<300; k++) {
                if (i2cdev_list[i].probe == 1) {
                    short unsigned int i2c_address[2]={i2cdev_list[i].board_info[j].addr, I2C_CLIENT_END};
                    e = i2c_new_probed_device(adap, &i2cdev_list[i].board_info[j] ,i2c_address, NULL);
                } else {
                    e = i2c_new_device(adap, &i2cdev_list[i].board_info[j]);
                }
                if(e == NULL) {
                    msleep(10);
                } else {
                    client_list[client_list_index] = e;
                    client_list_index++;
                    break;
                }
            }
            if(k==300) {
                printk("[%d][%d] i2c device not exist.\n",i,j);
            }
        }
        i2c_put_adapter(adap);
    }
    printk("inv_platform_init done\n");
    return ret;
}

static void __exit inv_platform_exit(void)
{
    int i;

    for(i=client_list_index-1; i>=0; i--) {
        i2c_unregister_device(client_list[i]);
    }
    device_i2c_gpio0->dev.platform_data = NULL;
    platform_device_unregister(device_i2c_gpio0);
    printk("inv_platform_exit done\n");
}

module_init(inv_platform_init);
module_exit(inv_platform_exit);

MODULE_AUTHOR("Inventec");
MODULE_DESCRIPTION("Platform devices");
MODULE_LICENSE("GPL");

