/*******************************************************************************
 * Copyright (c) 2015-2020 Barefoot Networks, Inc.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * $Id: $
 *
 ******************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "bf_fpga_ioctl.h"
#include "bf_pltfm_fpga.h"

/* ACCTON:Standalone Compile */
#define BF_FPGA_MAX_CNT 1

/* From bf_pltfm_types.h */
#define BF_PLTFM_STATUS_VALUES \
    BF_PLTFM_STATUS_(BF_PLTFM_SUCCESS, "Success"), \
    BF_PLTFM_STATUS_(BF_PLTFM_INVALID_ARG, "Invalid Arguments"), \
    BF_PLTFM_STATUS_(BF_PLTFM_OBJECT_NOT_FOUND, "Object Not Found"), \
    BF_PLTFM_STATUS_(BF_PLTFM_COMM_FAILED, \
                       "Communication Failed with Hardware"), \
    BF_PLTFM_STATUS_(BF_PLTFM_OBJECT_ALREADY_EXISTS, \
                       "Object Already Exists"), \
    BF_PLTFM_STATUS_(BF_PLTFM_OTHER, "Other")

enum bf_pltfm_status_enum
{
#define BF_PLTFM_STATUS_(x, y) x
    BF_PLTFM_STATUS_VALUES,
    BF_PLTFM_STS_MAX
#undef BF_PLTFM_STATUS_
};

typedef struct 
{
    int dev_fd;
    uint8_t *base_addr;
    bool inited;
} bf_fpga_dev_info_t;

static bf_fpga_dev_info_t bf_fpga_fd[BF_FPGA_MAX_CNT] =
{
    {-1, NULL, false},
};

void bf_fpga_reg_write32(int fd, uint32_t offset, uint32_t val)
{
    if (fd > BF_FPGA_MAX_CNT || offset > BF_FPGA_BAR_SIZE(0))
    {
        return;
    }

    *(uint32_t *)(bf_fpga_fd[fd].base_addr + offset) = val;
}

void bf_fpga_reg_write8(int fd, uint32_t offset, uint8_t val)
{
    if (fd > BF_FPGA_MAX_CNT || offset > BF_FPGA_BAR_SIZE(0))
    {
        return;
    }

    *(uint8_t *)(bf_fpga_fd[fd].base_addr + offset) = val;
}

int bf_fpga_reg_read8(int fd, uint32_t offset, uint8_t *val)
{
    if (fd > BF_FPGA_MAX_CNT || offset > BF_FPGA_BAR_SIZE(0))
    {
        return -1;
    }

    *val = *(uint8_t *)(bf_fpga_fd[fd].base_addr + offset);

    return 0;
}

int bf_fpga_reg_read32(int fd, uint32_t offset, uint32_t *val)
{
    if (fd > BF_FPGA_MAX_CNT || offset > BF_FPGA_BAR_SIZE(0))
    {
        return -1;
    }

    *val = *(uint32_t *)(bf_fpga_fd[fd].base_addr + offset);

    return 0;
}

int bf_pltfm_fpga_init(void *arg)
{
    int dev_fd, fpga_id;
    uint8_t *base_addr;
    char fpga_name[32];

    fpga_id = (int)(uintptr_t)arg;
    snprintf(fpga_name, sizeof(fpga_name), "%s%d", "/dev/bf_fpga_", fpga_id);
    dev_fd = open(fpga_name, O_RDWR);
    if (dev_fd < 0)
    {
        return -1;
    }
    base_addr = mmap(NULL, BF_FPGA_BAR_SIZE(0), PROT_READ 
                    | PROT_WRITE, MAP_SHARED, dev_fd, 0);

    if (base_addr == (uint8_t *)-1)
    {
        close(dev_fd);
        return -1;
    }

    bf_fpga_fd[fpga_id].dev_fd = dev_fd;
    bf_fpga_fd[fpga_id].base_addr = base_addr;
    bf_fpga_fd[fpga_id].inited = true;

    return 0;
}

void bf_pltfm_fpga_deinit(void *arg)
{
    int fpga_id;

    fpga_id = (int)(uintptr_t)arg;
    if (fpga_id >= BF_FPGA_MAX_CNT)
    {
        return;
    }

    if (bf_fpga_fd[fpga_id].dev_fd < 0 || !bf_fpga_fd[fpga_id].base_addr)
    {
        return;
    }

    munmap(bf_fpga_fd[fpga_id].base_addr, BF_FPGA_BAR_SIZE(0));

    close(bf_fpga_fd[fpga_id].dev_fd);

    bf_fpga_fd[fpga_id].dev_fd = -1;
    bf_fpga_fd[fpga_id].base_addr = NULL;
    bf_fpga_fd[fpga_id].inited = false;
}

static void populate_i2c_inst_ctrl_fields(bf_fpga_i2c_t *i2c_op, int inst_id,
                                          bool preemt,bool en, unsigned char i2c_addr,
                                          unsigned char i2c_type, unsigned char delay,
                                          unsigned char wr_cnt, unsigned char rd_cnt)
{
    i2c_op->i2c_inst[inst_id].preemt = preemt;
    i2c_op->i2c_inst[inst_id].en = en;
    i2c_op->i2c_inst[inst_id].i2c_addr = i2c_addr;
    i2c_op->i2c_inst[inst_id].i2c_type = i2c_type;
    i2c_op->i2c_inst[inst_id].delay = delay;
    i2c_op->i2c_inst[inst_id].wr_cnt = wr_cnt;
    i2c_op->i2c_inst[inst_id].rd_cnt = rd_cnt;
}

int bf_fpga_i2c_write(int fd, int bus, uint8_t delay, uint8_t addr,
                        uint8_t *wr_buf, uint8_t wr_sz)
{
    bf_fpga_i2c_t i2c_op;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || !wr_sz || !wr_buf || bus >= BF_I2C_FPGA_NUM_CTRL)
    {
        /* BF_PLTFM_COMM_FAILED */
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_op.one_time = 1;
    i2c_op.num_i2c = 1;
    i2c_op.inst_hndl.bus_id = bus;
    /* populate i2c operation */
    populate_i2c_inst_ctrl_fields(&i2c_op, 0, false, true, addr, 
                                    BF_FPGA_I2C_WRITE, delay, wr_sz, 0);
    memcpy(i2c_op.i2c_inst[0].wr_buf, wr_buf, wr_sz);
    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_ONETIME, &i2c_op);
    if (ret)
    {
        /* errorno has the actual system error code */
        ret = BF_PLTFM_COMM_FAILED; 
    }

    return ret;
}

int bf_fpga_i2c_write_byte(int fd, int bus, uint8_t delay, uint8_t addr, uint8_t val)
{
    return bf_fpga_i2c_write(fd, bus, delay, addr, &val, 1);
}

int bf_fpga_i2c_addr_read(int fd, int bus, uint8_t delay, uint8_t addr, 
                            uint8_t *wr_buf, uint8_t *rd_buf, uint8_t wr_sz,
                            uint8_t rd_sz)
{
    bf_fpga_i2c_t i2c_op;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || !wr_sz || !rd_sz || !wr_buf || !rd_buf 
                ||bus >= BF_I2C_FPGA_NUM_CTRL)
    {
        /* BF_PLTFM_COMM_FAILED */
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_op.one_time = 1;
    i2c_op.num_i2c = 1;
    i2c_op.inst_hndl.bus_id = bus;
    /* populate i2c operation */
    populate_i2c_inst_ctrl_fields(&i2c_op, 0, false, true, addr, BF_FPGA_I2C_ADDR_READ,
                                    delay, wr_sz, rd_sz);

    memcpy(i2c_op.i2c_inst[0].wr_buf, wr_buf, wr_sz);
    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_ONETIME, &i2c_op);
    if (ret)
    {
        /* errorno has the actual system error code */
        ret = BF_PLTFM_COMM_FAILED;
    }
    else 
    {
        memcpy(rd_buf, i2c_op.i2c_inst[0].rd_buf, rd_sz);
    }

    return ret;
}

int bf_fpga_i2c_read(int fd, int bus, uint8_t delay, uint8_t addr
                        , uint8_t *rd_buf, uint8_t rd_sz)
{
    bf_fpga_i2c_t i2c_op;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || !rd_sz || !rd_buf || bus >= BF_I2C_FPGA_NUM_CTRL)
    {
        /* BF_PLTFM_COMM_FAILED */
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_op.one_time = 1;
    i2c_op.num_i2c = 1;
    i2c_op.inst_hndl.bus_id = bus;
    /* populate i2c operation */
    populate_i2c_inst_ctrl_fields(&i2c_op, 0, false, true, addr, BF_FPGA_I2C_READ
                                    , delay, 0, rd_sz);
    memset(i2c_op.i2c_inst[0].rd_buf, 0, rd_sz);
    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_ONETIME, &i2c_op);
    if (ret)
    {
        /* errorno has the actual system error code */
        ret = BF_PLTFM_COMM_FAILED;
    }
    else
    {
        memcpy(rd_buf, i2c_op.i2c_inst[0].rd_buf, rd_sz);
    }

    return ret;
}

int bf_fpga_i2c_write_mux(int fd, int bus, uint8_t delay, uint8_t mux_addr,
                            uint8_t mux_chn, uint8_t i2c_addr, uint8_t *wr_buf,
                            uint8_t wr_sz)
{
    bf_fpga_i2c_t i2c_op;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || !wr_sz || !wr_buf ||bus >= BF_I2C_FPGA_NUM_CTRL 
                || mux_addr > 0x7F)
    {
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_op.one_time = 1;
    i2c_op.num_i2c = 3;
    i2c_op.inst_hndl.bus_id = bus;
    /* populate first operation -> set mux channel */
    populate_i2c_inst_ctrl_fields(&i2c_op, 0, true, true, mux_addr, BF_FPGA_I2C_WRITE
                                    , delay, 1, 0);
    i2c_op.i2c_inst[0].wr_buf[0] = mux_chn;
    /* populate second operation -> perform data transfer */
    populate_i2c_inst_ctrl_fields(&i2c_op, 1, true, true, i2c_addr, BF_FPGA_I2C_WRITE
                                    , 0, wr_sz, 0);
    memcpy(i2c_op.i2c_inst[1].wr_buf, wr_buf, wr_sz);
    /* populate third operation -> reset mux channel */
    populate_i2c_inst_ctrl_fields(&i2c_op, 2, false, true, mux_addr, BF_FPGA_I2C_WRITE
                                    , 0, 1, 0);
    /* disable all mux channels */
    i2c_op.i2c_inst[2].wr_buf[0] = 0x00;

    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_ONETIME, &i2c_op);
    if (ret)
    {
        /* errorno has the actual system error code */
        ret = BF_PLTFM_COMM_FAILED; 
    }

    return ret;
}

int bf_fpga_i2c_read_mux(int fd, int bus, uint8_t delay, uint8_t mux_addr,
                            uint8_t mux_chn, uint8_t i2c_addr, uint8_t *rd_buf,
                            uint8_t rd_sz)
{
    bf_fpga_i2c_t i2c_op;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || !rd_buf || !rd_sz || bus >= BF_I2C_FPGA_NUM_CTRL 
                || mux_addr > 0x7F)
    {
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_op.one_time = 1;
    i2c_op.num_i2c = 3;
    i2c_op.inst_hndl.bus_id = bus;
    /* populate first operation -> set mux channel */
    populate_i2c_inst_ctrl_fields(&i2c_op, 0, true, true, mux_addr, BF_FPGA_I2C_WRITE
                                    , delay, 1, 0);
    i2c_op.i2c_inst[0].wr_buf[0] = mux_chn;
    /* populate second operation -> perform data transfer */
    populate_i2c_inst_ctrl_fields(&i2c_op, 1, true, true, i2c_addr, BF_FPGA_I2C_READ
                                    , 0, 0, rd_sz);
    memset(i2c_op.i2c_inst[1].rd_buf, 0, rd_sz);
    /* populate third operation -> reset mux channel */
    populate_i2c_inst_ctrl_fields(&i2c_op, 2, false, true, mux_addr, BF_FPGA_I2C_WRITE
                                    , 0, 1, 0);
    /* disable all mux channels */
    i2c_op.i2c_inst[2].wr_buf[0] = 0x00;

    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_ONETIME, &i2c_op);
    if (ret)
    {
        /* errorno has the actual system error code */
        ret = BF_PLTFM_COMM_FAILED;
    }
    else 
    {
        memcpy(rd_buf, i2c_op.i2c_inst[1].rd_buf, rd_sz);
    }

    return ret;
}

int bf_fpga_i2c_addr_read_mux(int fd, int bus, uint8_t delay, uint8_t mux_addr,
                                uint8_t mux_chn, uint8_t i2c_addr, uint8_t *wr_buf,
                                uint8_t *rd_buf, uint8_t wr_sz, uint8_t rd_sz)
{
    bf_fpga_i2c_t i2c_op;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || !wr_sz || !rd_sz || !wr_buf || !rd_buf ||
                bus >= BF_I2C_FPGA_NUM_CTRL || mux_addr > 0x7F)
    {
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_op.one_time = 1;
    i2c_op.num_i2c = 3;
    i2c_op.inst_hndl.bus_id = bus;
    /* populate first operation -> set mux channel */
    populate_i2c_inst_ctrl_fields(&i2c_op, 0, true, true, mux_addr, BF_FPGA_I2C_WRITE
                                    , delay, 1, 0);
    i2c_op.i2c_inst[0].wr_buf[0] = mux_chn;
    /* populate second operation -> perform data transfer */
    populate_i2c_inst_ctrl_fields(&i2c_op, 1, true, true, i2c_addr, BF_FPGA_I2C_ADDR_READ
                                    , 0, wr_sz, rd_sz);
    memcpy(i2c_op.i2c_inst[1].wr_buf, wr_buf, wr_sz);
    /* populate third operation -> reset mux channel */
    populate_i2c_inst_ctrl_fields(&i2c_op, 2, false, true, mux_addr, BF_FPGA_I2C_WRITE
                                    , 0, 1, 0);
    /* disable all mux channels */
    i2c_op.i2c_inst[2].wr_buf[0] = 0x00;

    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_ONETIME, &i2c_op);
    if (ret)
    {   /* errorno has the actual system error code */
        ret = BF_PLTFM_COMM_FAILED;
    }
    else
    {
        memcpy(rd_buf, i2c_op.i2c_inst[1].rd_buf, rd_sz);
    }

    return ret;
}

int bf_fpga_i2c_write_add_pr(int fd, int bus, uint8_t delay, uint8_t i2c_addr,
                                uint8_t *wr_buf, uint8_t wr_sz, int *inst_id)
{
    bf_fpga_i2c_t i2c_op;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || !wr_sz || !wr_buf || bus >= BF_I2C_FPGA_NUM_CTRL)
    {
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_op.one_time = 0;
    i2c_op.num_i2c = 1;
    i2c_op.inst_hndl.bus_id = bus;
    /* populate the i2c operation */
    populate_i2c_inst_ctrl_fields(&i2c_op, 0, false, true, i2c_addr, BF_FPGA_I2C_WRITE
                                    , delay, wr_sz, 0);
    memcpy(i2c_op.i2c_inst[1].wr_buf, wr_buf, wr_sz);

    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_ADD_PR, &i2c_op);
    if (ret)
    {
        /* errorno has the actual system error code */
        ret = BF_PLTFM_COMM_FAILED;
        *inst_id = -1;
    }
    else 
    {
        *inst_id = i2c_op.inst_hndl.inst_id;
    }

    return ret;
}

int bf_fpga_i2c_read_add_pr(int fd, int bus, uint8_t delay, uint8_t i2c_addr,
                                uint8_t rd_sz, int *inst_id)
{
    bf_fpga_i2c_t i2c_op;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || !rd_sz || bus >= BF_I2C_FPGA_NUM_CTRL)
    {
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_op.one_time = 0;
    i2c_op.num_i2c = 1;
    i2c_op.inst_hndl.bus_id = bus;

    /* populate the i2c operation */
    populate_i2c_inst_ctrl_fields(&i2c_op, 0, false, true, i2c_addr, BF_FPGA_I2C_READ
                                    , delay, 0, rd_sz);

    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_ADD_PR, &i2c_op);
    if (ret)
    {
        /* errorno has the actual system error code */
        ret = BF_PLTFM_COMM_FAILED;
        *inst_id = -1;
    }
    else
    {
        *inst_id = i2c_op.inst_hndl.inst_id;
    }

    return ret;
}

int bf_fpga_i2c_addr_read_add_pr(int fd, int bus, uint8_t delay, uint8_t i2c_addr,
                                    uint8_t *wr_buf, uint8_t wr_sz, uint8_t rd_sz,
                                    int *inst_id)
{
    bf_fpga_i2c_t i2c_op;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || !wr_sz || !rd_sz || !wr_buf || bus >= BF_I2C_FPGA_NUM_CTRL)
    {
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_op.one_time = 0;
    i2c_op.num_i2c = 1;
    i2c_op.inst_hndl.bus_id = bus;
    /* populate the i2c operation */
    populate_i2c_inst_ctrl_fields(&i2c_op, 0, false, true, i2c_addr, BF_FPGA_I2C_ADDR_READ
                                , delay, wr_sz, rd_sz);
    memcpy(i2c_op.i2c_inst[1].wr_buf, wr_buf, wr_sz);

    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_ADD_PR, &i2c_op);
    if (ret)
    {
        /* errorno has the actual system error code */
        ret = BF_PLTFM_COMM_FAILED;
        *inst_id = -1;
    }
    else
    {
        *inst_id = i2c_op.inst_hndl.inst_id;
    }

    return ret;
}

int bf_fpga_i2c_write_add_pr_mux(int fd, int bus, uint8_t delay, uint8_t mux_i2c_addr, 
                                    uint8_t mux_chn, uint8_t i2c_addr, uint8_t *wr_buf, 
                                    uint8_t wr_sz, int *inst_id)
{
    bf_fpga_i2c_t i2c_op;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || !wr_sz || !wr_buf || bus >= BF_I2C_FPGA_NUM_CTRL 
                || mux_i2c_addr > 0x7F)
    {
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_op.one_time = 0;
    i2c_op.num_i2c = 3;
    i2c_op.inst_hndl.bus_id = bus;
    /* populate first operation -> set mux channel */
    populate_i2c_inst_ctrl_fields(&i2c_op, 0, true, true, mux_i2c_addr, BF_FPGA_I2C_WRITE
                                    , delay, 1, 0);
    i2c_op.i2c_inst[0].wr_buf[0] = mux_chn;
    /* populate second operation -> perform data transfer */
    populate_i2c_inst_ctrl_fields( &i2c_op, 1, true, true, i2c_addr, BF_FPGA_I2C_WRITE
                                    , 0, wr_sz, 0);
    memcpy(i2c_op.i2c_inst[1].wr_buf, wr_buf, wr_sz);
    /* populate third operation -> reset mux channel */
    populate_i2c_inst_ctrl_fields(&i2c_op, 2, false, true, mux_i2c_addr, BF_FPGA_I2C_WRITE
                                    , 0, 1, 0);
    /* disable all mux channels */
    i2c_op.i2c_inst[2].wr_buf[0] = 0x00;

    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_ADD_PR, &i2c_op);
    if (ret)
    {
        /* errorno has the actual system error code */
        ret = BF_PLTFM_COMM_FAILED;
        *inst_id = -1;
    }
    else
    {
        *inst_id = i2c_op.inst_hndl.inst_id;
    }

    return ret;
}

int bf_fpga_i2c_read_add_pr_mux(int fd, int bus, uint8_t delay, uint8_t mux_i2c_addr,
                                    uint8_t mux_chn, uint8_t i2c_addr, uint8_t rd_sz,
                                    int *inst_id)
{
    bf_fpga_i2c_t i2c_op;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || !rd_sz || bus >= BF_I2C_FPGA_NUM_CTRL 
                || mux_i2c_addr > 0x7F)
    {
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_op.one_time = 0;
    i2c_op.num_i2c = 3;
    i2c_op.inst_hndl.bus_id = bus;
    /* populate first operation -> set mux channel */
    populate_i2c_inst_ctrl_fields(&i2c_op, 0, true, true, mux_i2c_addr, BF_FPGA_I2C_WRITE
                                    , delay, 1, 0);
    i2c_op.i2c_inst[0].wr_buf[0] = mux_chn;
    /* populate second operation -> perform data transfer */
    populate_i2c_inst_ctrl_fields(&i2c_op, 1, true, true, i2c_addr, BF_FPGA_I2C_READ
                                    , 0, 0, rd_sz);
    /* populate third operation -> reset mux channel */
    populate_i2c_inst_ctrl_fields(&i2c_op, 2, false, true, mux_i2c_addr, BF_FPGA_I2C_WRITE
                                    , 0, 1, 0);
    /* disable all mux channels */
    i2c_op.i2c_inst[2].wr_buf[0] = 0x00;

    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_ADD_PR, &i2c_op);
    if (ret)
    {
        ret = BF_PLTFM_COMM_FAILED; /* errorno has the actual system error code */
        *inst_id = -1;
    }
    else
    {
        *inst_id = i2c_op.inst_hndl.inst_id;
    }

    return ret;
}

int bf_fpga_i2c_addr_read_add_pr_mux(int fd, int bus, uint8_t delay, uint8_t mux_i2c_addr,
                                        uint8_t mux_chn, uint8_t i2c_addr, uint8_t *wr_buf,
                                        uint8_t wr_sz, uint8_t rd_sz, int *inst_id)
{
    bf_fpga_i2c_t i2c_op;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || !wr_sz || !rd_sz || !wr_buf || bus >= BF_I2C_FPGA_NUM_CTRL 
                || mux_i2c_addr > 0x7F)
    {
        return BF_PLTFM_INVALID_ARG;
    }
    i2c_op.one_time = 0;
    i2c_op.num_i2c = 3;
    i2c_op.inst_hndl.bus_id = bus;
    /* populate first operation -> set mux channel */
    populate_i2c_inst_ctrl_fields(&i2c_op, 0, true, true, mux_i2c_addr, BF_FPGA_I2C_WRITE
                                    , delay, 1, 0);
    i2c_op.i2c_inst[0].wr_buf[0] = mux_chn;
    /* populate second operation -> perform data transfer */
    populate_i2c_inst_ctrl_fields(&i2c_op, 1, true, true, i2c_addr, BF_FPGA_I2C_ADDR_READ
                                    , 0, wr_sz, rd_sz);
    memcpy(i2c_op.i2c_inst[1].wr_buf, wr_buf, wr_sz);
    /* populate third operation -> reset mux channel */
    populate_i2c_inst_ctrl_fields(&i2c_op, 2, false, true, mux_i2c_addr, BF_FPGA_I2C_WRITE
                                    , 0, 1, 0);
    /* disable all mux channels */
    i2c_op.i2c_inst[2].wr_buf[0] = 0x00;

    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_ADD_PR, &i2c_op);
    if (ret)
    {
        /* errorno has the actual system error code */
        ret = BF_PLTFM_COMM_FAILED;
        *inst_id = -1;
    }
    else
    {
        *inst_id = i2c_op.inst_hndl.inst_id;
    }

    return ret;
}

static int bf_fpga_i2c_del_pr_internal(int fd, int bus, int inst_id, uint8_t num_i2c)
{
    bf_fpga_i2c_t i2c_op;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || bus >= BF_I2C_FPGA_NUM_CTRL 
        || inst_id < FPGA_I2C_PERIODIC_BEGIN_INDEX || inst_id >= FPGA_I2C_NUM_INST)
    {
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_op.num_i2c = num_i2c;
    i2c_op.inst_hndl.bus_id = bus;
    i2c_op.inst_hndl.inst_id = inst_id;

    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_DEL_PR, &i2c_op);
    if (ret)
    {
        /* errorno has the actual system error code */
        ret = BF_PLTFM_COMM_FAILED;
    }

    return ret;
}

int bf_fpga_i2c_del_pr(int fd, int bus, int inst_id)
{
    return bf_fpga_i2c_del_pr_internal(fd, bus, inst_id, 1);
}

int bf_fpga_i2c_del_pr_mux(int fd, int bus, int inst_id)
{
    return bf_fpga_i2c_del_pr_internal(fd, bus, inst_id, 3);
}

int bf_fpga_i2c_read_data(int fd, int bus, int inst_id, uint8_t *rd_buf
                            , uint8_t rd_sz, uint8_t offset)
{
    bf_fpga_i2c_rd_data_t i2c_data;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || bus >= BF_I2C_FPGA_NUM_CTRL || inst_id < 0 
                || inst_id >= FPGA_I2C_NUM_INST || !rd_sz || !rd_buf)
    {
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_data.inst_hndl.bus_id = bus;
    i2c_data.inst_hndl.inst_id = inst_id;
    i2c_data.offset = offset;
    i2c_data.rd_cnt = rd_sz;

    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_RD_DATA, &i2c_data);
    if (ret)
    {
        /* errorno has the actual system error code */
        ret = BF_PLTFM_COMM_FAILED;
        memset(rd_buf, 0, rd_sz);
    }
    else
    {
        memcpy(rd_buf, i2c_data.rd_buf, rd_sz);
    }

    return ret;
}

/* clk in hz */
int bf_fpga_i2c_set_clk(int fd, int bus, int clk)
{
    bf_fpga_i2c_set_clk_t i2c_clk;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || bus >= BF_I2C_FPGA_NUM_CTRL)
    {
        return BF_PLTFM_INVALID_ARG;
    }
    if (clk == 100000)
    {
        i2c_clk.clock_div = 0;
    }
    else if (clk == 400000)
    {
        i2c_clk.clock_div = 1;
    }
    else
    {
        i2c_clk.clock_div = 125000000 / (clk * 3);
    }

    i2c_clk.bus_id = bus;
    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_SET_CLK, &i2c_clk);
    return ret;
}

int bf_fpga_i2c_start(int fd, int bus, bool start)
{
    bf_fpga_i2c_ctl_t i2c_ctl;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || bus >= BF_I2C_FPGA_NUM_CTRL)
    {
        return BF_PLTFM_INVALID_ARG;
    }
    i2c_ctl.inst_hndl.bus_id = bus;
    if (start)
    {
        i2c_ctl.control_type = BF_FPGA_I2C_START;
    }
    else
    {
        i2c_ctl.control_type = BF_FPGA_I2C_STOP;
    }
    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_CTL, &i2c_ctl);

    return ret;
}

int bf_fpga_i2c_is_running(int fd, int bus, bool *running)
{
    bf_fpga_i2c_ctl_t i2c_ctl;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || bus >= BF_I2C_FPGA_NUM_CTRL)
    {
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_ctl.inst_hndl.bus_id = bus;
    i2c_ctl.control_type = BF_FPGA_I2C_BUSY;
    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_CTL, &i2c_ctl);
    *running = i2c_ctl.is_busy;

    return ret;
}

int bf_fpga_i2c_reset(int fd, int bus)
{
    bf_fpga_i2c_ctl_t i2c_ctl;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || bus >= BF_I2C_FPGA_NUM_CTRL)
    {
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_ctl.inst_hndl.bus_id = bus;
    i2c_ctl.control_type = BF_FPGA_I2C_RESET;
    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_CTL, &i2c_ctl);

    return ret;
}

int bf_fpga_i2c_en(int fd, int bus, int inst_id, bool en)
{
    bf_fpga_i2c_ctl_t i2c_ctl;
    int ret;

    if (fd >= BF_FPGA_MAX_CNT || bus >= BF_I2C_FPGA_NUM_CTRL) {
        return BF_PLTFM_INVALID_ARG;
    }

    i2c_ctl.inst_hndl.bus_id = bus;
    i2c_ctl.inst_hndl.inst_id = inst_id;
    if (en) 
    {
        i2c_ctl.control_type = BF_FPGA_I2C_INST_EN;
    }
    else
    {
        i2c_ctl.control_type = BF_FPGA_I2C_INST_DIS;
    }

    ret = ioctl(bf_fpga_fd[fd].dev_fd, BF_FPGA_IOCTL_I2C_CTL, &i2c_ctl);

    return ret;
}
