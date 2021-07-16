#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/gpio.h>

static int custom_match_dev(struct device *dev, void *data)
{
    const char *name = data;

    return sysfs_streq(name, dev->kobj.name);
}

struct device *find_dev( const char *name, struct bus_type *bus)
{
    struct device *dev = bus_find_device(bus, NULL, (void *)name, custom_match_dev);

    return dev;
}
/*gpio base find */
static int gpiochip_match_name(struct gpio_chip *chip, void *data)
{
    const char *name = data;
    return !strcmp(chip->label, name);
}

static struct gpio_chip *find_chip_by_name(const char *name)
{
    return gpiochip_find((void *)name, gpiochip_match_name);
}        

int gpio_base_find(int *gpio_base)
{    
    struct gpio_chip *chip = NULL;
    
    if (NULL == (chip = find_chip_by_name("gpio_ich"))) {
        printk("find gpio name fail\n");
        return -EBADRQC;
    }
    *gpio_base = chip->base;
    return 0;
}
