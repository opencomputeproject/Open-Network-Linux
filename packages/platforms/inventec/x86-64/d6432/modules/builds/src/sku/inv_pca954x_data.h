#ifndef __INV_PCA954X_DATA_H
#define __INV_PCA954X_DATA_H

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,151)
#include <linux/platform_data/pca954x.h>
#else
#include <linux/i2c/pca954x.h>
#endif
struct pca954x {
    const struct chip_desc *chip;
    struct i2c_client *client;
    u64 last_chan;      /* last register value */
    u64 block;
    bool mux_fail;
    void  (*mux_fail_set)(struct device *dev, bool fail);
    void  (*mux_fail_get)(struct device *dev, bool *fail);
    void  (*current_channel_get)(struct device *dev, u64 *data);
    void  (*block_channel_set)(struct device *dev, u64 data);
    void  (*block_channel_get)(struct device *dev, u64 *data);
};
#endif /* __INV_PCA954X_DATA_H */



