/*******************************************************************************
 * Copyright (c) 2015-2020 Barefoot Networks, Inc.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * $Id: $
 *
 ******************************************************************************/
#ifndef _BF_PLTFM_FPGA_H
#define _BF_PLTFM_FPGA_H
/* Allow the use in C++ code. */
#ifdef __cplusplus
extern "C" {
#endif

#define BF_FPGA_MAX_CNT 1
#define BF_FPGA_BAR_SIZE(i) ((i == 0) ? (256 * 1024) : 0)
#define BF_FPGA_DEV_BASE_NAME "/dev/bf_fpga_"

// Move these to bf_newport_board.h - TBD
#define BF_NEWPORT_FPGA_MAX_ACCESS_LEN 64  /* Hack until fixed inside the driver */
#define BF_NEWPORT_FPGA_BUS_MISC 32
#define BF_NEWPORT_FPGA_BUS_CPLD 33

#define BF_NEWPORT_FPGA_MISC_MUX_ADDR 0x74
#define BF_NEWPORT_FPGA_MUX_CHN_QSFP_LP0 0x01
#define BF_NEWPORT_FPGA_MUX_CHN_QSFP_LP1 0x02
#define BF_NEWPORT_FPGA_MUX_CHN_QSFP_PRSNT0 0x04
#define BF_NEWPORT_FPGA_MUX_CHN_QSFP_PRSNT1 0x08
#define BF_NEWPORT_FPGA_MUX_CHN_QSFP_INT0 0x10
#define BF_NEWPORT_FPGA_MUX_CHN_QSFP_INT1 0x20
#define BF_NEWPORT_FPGA_MUX_CHN_QSFP_RST0 0x40
#define BF_NEWPORT_FPGA_MUX_CHN_QSFP_RST1 0x80
#define BF_NEWPORT_FPGA_MUX_CHN_SYNCE 0x40
#define BF_NEWPORT_FPGA_MUX_CHN_EEPROM 0x40
#define BF_NEWPORT_FPGA_MUX_CHN_TOFINO 0x40
#define BF_NEWPORT_FPGA_MUX_CHN_CPU_PORT 0x80
/* i2c reset control register offsets */
#define BF_NEWPORT_FPGA_MISC_BASE 0x3F000UL
#define BF_NEWPORT_FPGA_MISC_RST_CTRL_QSFP (BF_NEWPORT_FPGA_MISC_BASE + 0x08)
#define BF_NEWPORT_FPGA_MISC_RST_CTRL_MISC (BF_NEWPORT_FPGA_MISC_BASE + 0x0C)

#define BF_NEWPORT_FPGA_MUX_I2C_RST_SFT 0
#define BF_NEWPORT_FPGA_CPLD_I2C_RST_SFT 1

/* i2c slave devices */
enum {
    BF_NEWPORT_FPGA_I2CADDR_QSFP_LP0 = 0x20,
    BF_NEWPORT_FPGA_I2CADDR_QSFP_LP1,
    BF_NEWPORT_FPGA_I2CADDR_QSFP_PRSNT0,
    BF_NEWPORT_FPGA_I2CADDR_QSFP_PRSNT1,
    BF_NEWPORT_FPGA_I2CADDR_QSFP_INT0,
    BF_NEWPORT_FPGA_I2CADDR_QSFP_INT1,
    BF_NEWPORT_FPGA_I2CADDR_QSFP_RST0,
    BF_NEWPORT_FPGA_I2CADDR_QSFP_RST1,
};

#define BF_NP_SYSCPLD_I2C_ADDR (0x32)

int bf_pltfm_fpga_init(void *arg);
void bf_pltfm_fpga_deinit(void *arg);
void bf_fpga_reg_write32(int fd, uint32_t offset, uint32_t val);
void bf_fpga_reg_write8(int fd, uint32_t offset, uint8_t val);
int bf_fpga_reg_read32(int fd, uint32_t offset, uint32_t *val);
int bf_fpga_reg_read8(int fd, uint32_t offset, uint8_t *val);
int bf_fpga_i2c_write(int fd, int bus, uint8_t delay, uint8_t addr, 
                        uint8_t *wr_buf, uint8_t wr_sz);
int bf_fpga_i2c_write_byte(int fd, int bus, uint8_t delay, 
                            uint8_t addr, uint8_t val);

int bf_fpga_i2c_read(int fd, int bus, uint8_t delay, uint8_t addr, 
                        uint8_t *rd_buf, uint8_t rd_sz);

int bf_fpga_i2c_addr_read(int fd, int bus, uint8_t delay, uint8_t addr,  
                            uint8_t *wr_buf, uint8_t *rd_buf, uint8_t wr_sz, 
                            uint8_t rd_sz);

int bf_fpga_i2c_write_mux(int fd, int bus, uint8_t delay,
                            uint8_t mux_addr, uint8_t mux_chn,
                            uint8_t i2c_addr,uint8_t *wr_buf,
                            uint8_t wr_sz);

int bf_fpga_i2c_read_mux(int fd, int bus, uint8_t delay, uint8_t mux_addr,
                            uint8_t mux_chn, uint8_t i2c_addr, uint8_t *rd_buf,
                            uint8_t rd_sz);

int bf_fpga_i2c_addr_read_mux(int fd, int bus, uint8_t delay, uint8_t mux_addr,
                                uint8_t mux_chn, uint8_t i2c_addr, uint8_t *wr_buf,
                                uint8_t *rd_buf, uint8_t wr_sz, uint8_t rd_sz);

int bf_fpga_i2c_addr_read(int fd, int bus, uint8_t delay, uint8_t addr,
                            uint8_t *wr_buf, uint8_t *rd_buf, uint8_t wr_sz,
                            uint8_t rd_sz);

int bf_fpga_i2c_write_add_pr(int fd, int bus, uint8_t delay, uint8_t i2c_addr,
                                uint8_t *wr_buf, uint8_t wr_sz, int *inst_id);

int bf_fpga_i2c_read_add_pr(int fd, int bus, uint8_t delay, uint8_t i2c_addr,
                                uint8_t rd_sz, int *inst_id);

int bf_fpga_i2c_addr_read_add_pr(int fd, int bus, uint8_t delay, uint8_t i2c_addr,
                                    uint8_t *wr_buf, uint8_t wr_sz, uint8_t rd_sz,
                                    int *inst_id);

int bf_fpga_i2c_write_add_pr_mux(int fd, int bus, uint8_t delay, uint8_t mux_i2c_addr,
                                    uint8_t mux_chn, uint8_t i2c_addr, uint8_t *wr_buf,
                                    uint8_t wr_sz, int *inst_id);

int bf_fpga_i2c_read_add_pr_mux(int fd, int bus, uint8_t delay, uint8_t mux_i2c_addr,
                                    uint8_t mux_chn, uint8_t i2c_addr, uint8_t rd_sz,
                                    int *inst_id);

int bf_fpga_i2c_addr_read_add_pr_mux(int fd, int bus, uint8_t delay, uint8_t mux_i2c_addr,
                                        uint8_t mux_chn, uint8_t i2c_addr, uint8_t *wr_buf,
                                        uint8_t wr_sz, uint8_t rd_sz, int *inst_id);

int bf_fpga_i2c_del_pr(int fd, int bus, int inst_id);

int bf_fpga_i2c_del_pr_mux(int fd, int bus, int inst_id);

int bf_fpga_i2c_read_data(int fd, int bus, int inst_id, uint8_t *rd_buf, uint8_t rd_sz,
                            uint8_t offset);

int bf_fpga_i2c_set_clk(int fd, int bus, int clk);

int bf_fpga_i2c_start(int fd, int bus, bool start);

int bf_fpga_i2c_is_running(int fd, int bus, bool *running);

int bf_fpga_i2c_reset(int fd, int bus);

int bf_fpga_i2c_en(int fd, int bus, int inst_id, bool en);

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _BF_PLTFM_FPGA_H */
