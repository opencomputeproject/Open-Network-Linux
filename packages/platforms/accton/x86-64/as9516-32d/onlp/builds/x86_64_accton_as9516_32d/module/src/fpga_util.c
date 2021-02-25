#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <stdbool.h>
#include "bf_fpga_ioctl.h"
#include "bf_pltfm_fpga.h"

int reg_chnl = 0;
bool dbg_print = false;

static char *usage[] =
{
    "fpga_util i2c_read <fpga_id> <bus> <mux_i2c_adr> <mux_chn> <i2c_addr> "
    "<length>",
    "fpga_util i2c_write <fpga_id> <bus> <mux_i2c_adr> <mux_chn> <i2c_addr> "
    "<length> <byte1> [<byte2> ...]",
    "fpga_util i2c_addr_read <fpga_id> <bus> <mux_i2c_adr> <mux_chn> "
    "<i2c_addr> <read_length> <write_length> <byte1> [<byte2> ...]",
    "fpga_util reg_read <fpga_id> <reg_addr>"
    "fpga_util reg_write <fpga_id> <reg_addr> <32bit data>"
};

static int proc_i2c_read(int fd, int count, char *cmd[])
{
    uint8_t bus, i2c_addr, mux_i2c_addr, mux_chn;
    uint8_t rd_size, byte_buf[128];
    uint32_t i;
    int ret;

    if (count < 7)
    {
        printf("Error: Insufficient arguments\nUsage : %s\n", usage[0]);
        return -1;
    }

    bus = (uint8_t)strtol(cmd[0], NULL, 0);
    mux_i2c_addr = (uint8_t)strtol(cmd[1], NULL, 0);
    mux_chn = (uint8_t)strtol(cmd[2], NULL, 0);
    i2c_addr = (uint8_t)strtol(cmd[3], NULL, 0);
    rd_size = (uint8_t)strtol(cmd[4], NULL, 0);

    if (mux_i2c_addr >= 0x80)
    {
        ret = bf_fpga_i2c_read(fd, bus, 0, i2c_addr, byte_buf, rd_size);
    }
    else
    {
        ret = bf_fpga_i2c_read_mux(fd, bus, 0, mux_i2c_addr, mux_chn, i2c_addr
                                    , byte_buf, rd_size);
    }

    if (ret)
    {
        printf("Error: read failed for bus %hhd addr 0x%02x\n", bus, i2c_addr);
        return -1;
    }
    else
    {
        for (i = 0; i < rd_size; i++)
        {
            printf("%02x ", (unsigned)byte_buf[i]);
        }
        printf("\n");
    }

    return 0;
}

static int proc_i2c_write(int fd, int count, char *cmd[])
{
    uint8_t bus, i2c_addr, mux_i2c_addr, mux_chn;
    uint8_t wr_size, byte_buf[128];
    uint32_t i;
    int ret;

    if (count < 8)
    {
        printf("Error: Insufficient arguments\nUsage: %s\n", usage[1]);
        return -1;
    }

    bus = (uint8_t)strtol(cmd[0], NULL, 0);
    mux_i2c_addr = (uint8_t)strtol(cmd[1], NULL, 0);
    mux_chn = (uint8_t)strtol(cmd[2], NULL, 0);
    i2c_addr = (uint8_t)strtol(cmd[3], NULL, 0);
    wr_size = (uint8_t)strtol(cmd[4], NULL, 0);

    if (count < (int)(7 + wr_size))
    {
        printf("Error: Insufficient Arguments \nUsage: %s\n", usage[1]);
        return 0;
    }

    for (i = 0; i < wr_size; i++)
    {
        byte_buf[i] = strtol(cmd[5 + i], NULL, 0);
    }

    if (mux_i2c_addr >= 0x80)
    {
        ret = bf_fpga_i2c_write(fd, bus, 0, i2c_addr, byte_buf, wr_size);
    }
    else
    {
        ret = bf_fpga_i2c_write_mux(fd, bus, 0, mux_i2c_addr, mux_chn, i2c_addr
                                    , byte_buf, wr_size);
    }

    if (ret)
    {
        printf("Error: write failed for bus %hhd addr 0x%02x \n", bus, i2c_addr);
        return -1;
    }

    return 0;
}

static int proc_i2c_addr_read(int fd, int count, char *cmd[])
{
    uint8_t bus, i2c_addr, mux_i2c_addr, mux_chn;
    uint8_t wr_size, rd_size, byte_buf[128];
    uint32_t i;
    int ret;

    if (count < 9)
    {
        printf("Error: Insufficient Arguments \nUsage: %s\n", usage[2]);

        return 0;
    }

    bus = (uint8_t)strtol(cmd[0], NULL, 0);
    mux_i2c_addr = (uint8_t)strtol(cmd[1], NULL, 0);
    mux_chn = (uint8_t)strtol(cmd[2], NULL, 0);
    i2c_addr = (uint8_t)strtol(cmd[3], NULL, 0);
    rd_size = (uint8_t)strtol(cmd[4], NULL, 0);
    wr_size = (uint8_t)strtol(cmd[5], NULL, 0);

    if (count < (int)(8 + wr_size)) {

        printf("Error: Insufficient arguments\nUsage: %s\n", usage[2]);

        return -1;
    }

    for (i = 0; i < wr_size; i++)
    {
        byte_buf[i] = strtol(cmd[6 + i], NULL, 0);
    }

    if (mux_i2c_addr >= 0x80)
    {
        ret = bf_fpga_i2c_addr_read(fd, bus, 0, i2c_addr, byte_buf, byte_buf
                                    , wr_size, rd_size);
    }
    else
    {
        ret = bf_fpga_i2c_addr_read_mux(fd, bus, 0, mux_i2c_addr, mux_chn, i2c_addr
                                        , byte_buf, byte_buf, wr_size, rd_size);
    }

    if (ret) 
    {
        printf("Error: addr read failed for bus %hhd addr 0x%02x \n", bus, i2c_addr);

        return -1;
    }
    else
    {
        for (i = 0; i < rd_size; i++)
        {
            printf("%02x ", (unsigned)byte_buf[i]);
        }

        printf("\n");
    }

    return 0;
}

static int proc_reg_read(int fd, int count, char *cmd[])
{
    uint32_t reg, val;

    reg = strtol(cmd[0], NULL, 0);

    bf_fpga_reg_read32(fd, reg, &val);

    printf("register offset 0x%x is 0x%x\n", reg, val);

    return 0;
}

static int proc_reg_write(int fd, int count, char *cmd[])
{
    uint32_t reg, val;

    if (count < 4)
    {
        printf("Error: Insufficient arguments\nUsage : %s\n", usage[4]);
        return -1;
    }

    reg = strtol(cmd[0], NULL, 0);
    val = strtol(cmd[1], NULL, 0);

    bf_fpga_reg_write32(fd, reg, val);

    printf("wrote 0x%x at register offset 0x%x\n", val, reg);

    return 0;
}

static int process_request(int fd, int count, char *cmd[])
{
    if (strcmp(cmd[0], "i2c_read") == 0) 
    {
        return proc_i2c_read(fd, count, &cmd[2]);
    }
    else if (strcmp(cmd[0], "i2c_write") == 0)
    {
        return proc_i2c_write(fd, count, &cmd[2]);
    }
    else if (strcmp(cmd[0], "i2c_addr_read") == 0)
    {
        return proc_i2c_addr_read(fd, count, &cmd[2]);
    }
    else if (strcmp(cmd[0], "reg_read") == 0)
    {
        return proc_reg_read(fd, count, &cmd[2]);
    }
    else if (strcmp(cmd[0], "reg_write") == 0)
    {
        return proc_reg_write(fd, count, &cmd[2]);
    }
    else
    {
        printf("Error: Invalid command\n");
        return -1;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        uint32_t i = 0;

        printf("Usage: \n");

        for (i = 0; i < (sizeof(usage) / sizeof(usage[0])); i++)
        {
            printf("%s\n", usage[i]);
        }

        return 1;
    }

    uint8_t fpga_id = (uint8_t)strtol(argv[2], NULL, 0);
    if (bf_pltfm_fpga_init((void *)(uintptr_t)fpga_id))
    {
        printf("Error: Not able to initialize the fpga device\n");
        return 1;
    }

    int ret = process_request(fpga_id, argc - 1, &argv[1]);

    bf_pltfm_fpga_deinit((void *)(uintptr_t)fpga_id);

    return ret;
}
