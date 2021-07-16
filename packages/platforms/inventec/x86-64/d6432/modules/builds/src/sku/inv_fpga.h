// Copyright 2014 Raytheon BBN Technologies
// Original Author: Colm Ryan (cryan@bbn.com)

/*
 * for i2c, please refer: Xilinx pg090
 * for bram, please refer: Xilinx pg078
 * for spi, please refer: Xilinx pg153
 * for gpio, please refer: Xilinx pg144
 * for pcie, please refer: Xilinx pg054
 * for platform bar info/ gpio layout, contact fpga engineer
 *
 */

#define VENDOR_ID 0x10ee
#define DEVICE_ID 0x7011
#define DEVICE_GEN2_ID 0x7021

#define BAR_i2c 0
#define BAR_intc 1
#define BAR_bram 2

#define MAX_IIC_DEV 32

#define EEPROM_ADDR		0x50
#define EEPROM_PAGE_OFFSET		0x7f
//#define BRAM_SUPPORT

#define MSIX_SUPPORT
#define MAX_MSIX_ENTRIES 100

/* gpio ip definition */
#define AXI_GPIO_DEV	4
#define AXI_GPIO_INPUT	1
/* FIXME: assume only one and always be first one for input signal here */
#define IDX_GPIO_INPUT	0
/* FIXME: need to check the gpio mapping for sff output signal */
#define IDX_SFF_PRS			0	/* ip # of this signal */
#define IDX_SFF_INTR		0
#define IDX_SFF_LPMOD		1
#define IDX_SFF_RESET		1
#define IDX_SFF_MODSEL		2
#define CHAN_SFF_PRS		1	/* chan # of this signal */
#define CHAN_SFF_INTR		2
#define CHAN_SFF_LPMOD		1
#define CHAN_SFF_RESET		2
#define CHAN_SFF_MODSEL		1

enum axi_gpio_chan {
	AXI_GPIO_CH1 = 1,
	AXI_GPIO_CH2
};

/* gpio ip register */
#define AXI_GPIO_DATA	0x0
#define AXI_GPIO_TRI	0x4
#define AXI_GPIO2_DATA	0x8
#define AXI_GPIO2_TRI	0xc
#define AXI_GPIO_GIER	0x11c
#define AXI_GPIO_IPISR	0x120
#define AXI_GPIO_IPIER	0x128

#define AXI_GPIO_GIER_ON	(0x1 << 31)
#define AXI_GPIO_IPIER_CHAN1_ON	(0x1 << 0)
#define AXI_GPIO_IPIER_CHAN2_ON	(0x1 << 1)
#define AXI_GPIO_IPISR_CHAN1_ON	(0x1 << 0)
#define AXI_GPIO_IPISR_CHAN2_ON	(0x1 << 1)

/* bram part */
#define BRAM_PAGE_CAPACITY 10

typedef enum {
	work_idle		=	0x00,
	work_done		= 	0x01,
	work_running		=	0x02,
	work_fail		=	0x80
} work_status;

struct bram_eeprom_write {              //reserved
        uint8_t status;
        uint8_t page_number;
        uint8_t data_len;
        uint8_t data[64];
};

struct bram_page0_data {
	uint8_t status;			//work_status
	uint8_t data[256];		//store page0's 0x00-0xFF 256bytes
};

struct bram_pages_data {
	uint8_t page_number;		//need SWPS set the page number
	uint8_t status;			//work_status
	uint8_t data[129];		//store the page's 0x7F-0xFF (129bytes)
};

struct bram_port_data {
	uint8_t enable;	  		//port's pre-load polling (default: enable after present)
	uint8_t status;			//work_status
	struct bram_page0_data page0;
	struct bram_pages_data pages[BRAM_PAGE_CAPACITY];
	struct bram_eeprom_write write;	//reserved
};

struct bram_memory {
	uint8_t enable;			//pre-load polling (default: enable)
	uint16_t interval;		//interval unit: 0.1ms
	struct bram_port_data bram_ports[MAX_IIC_DEV];
};
typedef struct bram_memory bram_memory_t;
#define ENABLE()  offsetof(bram_memory_t,enable)
#define INTERVAL()  offsetof(bram_memory_t,interval)
#define PORTENABLE(port_index)  offsetof(bram_memory_t,bram_ports[port_index].enable)
#define PORTSTATUS(port_index)  offsetof(bram_memory_t,bram_ports[port_index].status)
#define PAGE0STATUS(port_index)          offsetof(bram_memory_t,bram_ports[port_index].page0.status)
#define PAGE0DATA(port_index,data_index) offsetof(bram_memory_t,bram_ports[port_index].page0.data[data_index])
#define PAGESNUMBER(port_index,page_index)           offsetof(bram_memory_t,bram_ports[port_index].pages[page_index].page_number)
#define PAGESSTATUS(port_index,page_index)           offsetof(bram_memory_t,bram_ports[port_index].pages[page_index].status)
#define PAGESDATA(port_index,page_index,data_index)  offsetof(bram_memory_t,bram_ports[port_index].pages[page_index].data[data_index])

/* bram part end*/
/* i2c part */
struct xiic_i2c_platform_data {
	u8	num_devices;
	struct i2c_board_info const	*devices;
};

enum xilinx_i2c_state {
	STATE_DONE,
	STATE_ERROR,
	STATE_START
};

enum xiic_endian {
	LITTLE,
	BIG
};

/**
 * struct xiic_i2c - Internal representation of the XIIC I2C bus
 * @dev:	Pointer to device structure
 * @base:	Memory base of the HW registers
 * @wait:	Wait queue for callers
 * @adap:	Kernel adapter representation
 * @tx_msg:	Messages from above to be sent
 * @lock:	Mutual exclusion
 * @tx_pos:	Current pos in TX message
 * @nmsgs:	Number of messages in tx_msg
 * @state:	See STATE_
 * @rx_msg:	Current RX message
 * @rx_pos:	Position within current RX message
 * @endianness: big/little-endian byte order
 * @clk:	Pointer to struct clk
 * @dynamic: Mode of controller
 * @repeated_start: Repeated start operation
 * @prev_msg_tx: Previous message is Tx
 */
struct xiic_i2c {
	struct device		*dev;
	void __iomem		*base;
	wait_queue_head_t	wait;
	struct i2c_adapter	adap;
	struct i2c_msg		*tx_msg;
	struct mutex		lock;
	unsigned int		tx_pos;
	unsigned int		nmsgs;
	enum xilinx_i2c_state	state;
	struct i2c_msg		*rx_msg;
	int			rx_pos;
	enum xiic_endian	endianness;
	struct clk *clk;
	bool dynamic;
	bool repeated_start;
	bool prev_msg_tx;
	int id;
};

#define XIIC_MSB_OFFSET 0x0
#define XIIC_REG_OFFSET (0x100 + XIIC_MSB_OFFSET)

/*
 * Register offsets in bytes from RegisterBase. Three is added to the
 * base offset to access LSB (IBM style) of the word
 */
#define XIIC_CR_REG_OFFSET   (0x00 + XIIC_REG_OFFSET)	/* Control Register   */
#define XIIC_SR_REG_OFFSET   (0x04 + XIIC_REG_OFFSET)	/* Status Register    */
#define XIIC_DTR_REG_OFFSET  (0x08 + XIIC_REG_OFFSET)	/* Data Tx Register   */
#define XIIC_DRR_REG_OFFSET  (0x0C + XIIC_REG_OFFSET)	/* Data Rx Register   */
#define XIIC_ADR_REG_OFFSET  (0x10 + XIIC_REG_OFFSET)	/* Address Register   */
#define XIIC_TFO_REG_OFFSET  (0x14 + XIIC_REG_OFFSET)	/* Tx FIFO Occupancy  */
#define XIIC_RFO_REG_OFFSET  (0x18 + XIIC_REG_OFFSET)	/* Rx FIFO Occupancy  */
#define XIIC_TBA_REG_OFFSET  (0x1C + XIIC_REG_OFFSET)	/* 10 Bit Address reg */
#define XIIC_RFD_REG_OFFSET  (0x20 + XIIC_REG_OFFSET)	/* Rx FIFO Depth reg  */
#define XIIC_GPO_REG_OFFSET  (0x24 + XIIC_REG_OFFSET)	/* Output Register    */

/* Control Register masks */
#define XIIC_CR_ENABLE_DEVICE_MASK        0x01	/* Device enable = 1      */
#define XIIC_CR_TX_FIFO_RESET_MASK        0x02	/* Transmit FIFO reset=1  */
#define XIIC_CR_MSMS_MASK                 0x04	/* Master starts Txing=1  */
#define XIIC_CR_DIR_IS_TX_MASK            0x08	/* Dir of tx. Txing=1     */
#define XIIC_CR_NO_ACK_MASK               0x10	/* Tx Ack. NO ack = 1     */
#define XIIC_CR_REPEATED_START_MASK       0x20	/* Repeated start = 1     */
#define XIIC_CR_GENERAL_CALL_MASK         0x40	/* Gen Call enabled = 1   */
#define XIIC_CR_COLLISION_MASK			  (XIIC_CR_MSMS_MASK | XIIC_CR_ENABLE_DEVICE_MASK)

/* Status Register masks */
#define XIIC_SR_GEN_CALL_MASK             0x01	/* 1=a mstr issued a GC   */
#define XIIC_SR_ADDR_AS_SLAVE_MASK        0x02	/* 1=when addr as slave   */
#define XIIC_SR_BUS_BUSY_MASK             0x04	/* 1 = bus is busy        */
#define XIIC_SR_MSTR_RDING_SLAVE_MASK     0x08	/* 1=Dir: mstr <-- slave  */
#define XIIC_SR_TX_FIFO_FULL_MASK         0x10	/* 1 = Tx FIFO full       */
#define XIIC_SR_RX_FIFO_FULL_MASK         0x20	/* 1 = Rx FIFO full       */
#define XIIC_SR_RX_FIFO_EMPTY_MASK        0x40	/* 1 = Rx FIFO empty      */
#define XIIC_SR_TX_FIFO_EMPTY_MASK        0x80	/* 1 = Tx FIFO empty      */

/* Interrupt Status Register masks    Interrupt occurs when...       */
#define XIIC_INTR_ARB_LOST_MASK           0x01	/* 1 = arbitration lost   */
#define XIIC_INTR_TX_ERROR_MASK           0x02	/* 1=Tx error/msg complete */
#define XIIC_INTR_TX_EMPTY_MASK           0x04	/* 1 = Tx FIFO/reg empty  */
#define XIIC_INTR_RX_FULL_MASK            0x08	/* 1=Rx FIFO/reg=OCY level */
#define XIIC_INTR_BNB_MASK                0x10	/* 1 = Bus not busy       */
#define XIIC_INTR_AAS_MASK                0x20	/* 1 = when addr as slave */
#define XIIC_INTR_NAAS_MASK               0x40	/* 1 = not addr as slave  */
#define XIIC_INTR_TX_HALF_MASK            0x80	/* 1 = TX FIFO half empty */

/* The following constants specify the depth of the FIFOs */
#define IIC_RX_FIFO_DEPTH         16	/* Rx fifo capacity               */
#define IIC_TX_FIFO_DEPTH         16	/* Tx fifo capacity               */

/* The following constants specify groups of interrupts that are typically
 * enabled or disables at the same time
 */
#define XIIC_TX_INTERRUPTS                           \
(XIIC_INTR_TX_ERROR_MASK | XIIC_INTR_TX_EMPTY_MASK | XIIC_INTR_TX_HALF_MASK)

#define XIIC_TX_RX_INTERRUPTS (XIIC_INTR_RX_FULL_MASK | XIIC_TX_INTERRUPTS)

/*
 * Tx Fifo upper bit masks.
 */
#define XIIC_TX_DYN_START_MASK            0x0100 /* 1 = Set dynamic start */
#define XIIC_TX_DYN_STOP_MASK             0x0200 /* 1 = Set dynamic stop */

/* Dynamic mode constants */
#define MAX_READ_LENGTH_DYNAMIC		255 /* Max length for dynamic read */

/*
 * The following constants define the register offsets for the Interrupt
 * registers. There are some holes in the memory map for reserved addresses
 * to allow other registers to be added and still match the memory map of the
 * interrupt controller registers
 */
#define XIIC_DGIER_OFFSET    0x1C /* Device Global Interrupt Enable Register */
#define XIIC_IISR_OFFSET     0x20 /* Interrupt Status Register */
#define XIIC_IIER_OFFSET     0x28 /* Interrupt Enable Register */
#define XIIC_RESETR_OFFSET   0x40 /* Reset Register */

#define XIIC_RESET_MASK             0xAUL

#define XIIC_PM_TIMEOUT		1000	/* ms */
/* timeout waiting for the controller to respond */
#define XIIC_I2C_TIMEOUT	(msecs_to_jiffies(1000))
/*
 * The following constant is used for the device global interrupt enable
 * register, to enable all interrupts for the device, this is the only bit
 * in the register
 */
#define XIIC_GINTR_ENABLE_MASK      0x80000000UL

#define xiic_tx_space(i2c) ((i2c)->tx_msg->len - (i2c)->tx_pos)
#define xiic_rx_space(i2c) ((i2c)->rx_msg->len - (i2c)->rx_pos)
/* i2c part end */

/* spi part */
#define XILINX_SPI_MAX_CS	32

/* Register definitions as per "OPB Serial Peripheral Interface (SPI) (v1.00e)
 * Product Specification", DS464
 */
#define XSPI_CR_OFFSET		0x60	/* Control Register */

#define XSPI_CR_LOOP		0x01
#define XSPI_CR_ENABLE		0x02
#define XSPI_CR_MASTER_MODE	0x04
#define XSPI_CR_CPOL		0x08
#define XSPI_CR_CPHA		0x10
#define XSPI_CR_MODE_MASK	(XSPI_CR_CPHA | XSPI_CR_CPOL | \
				 XSPI_CR_LSB_FIRST | XSPI_CR_LOOP)
#define XSPI_CR_TXFIFO_RESET	0x20
#define XSPI_CR_RXFIFO_RESET	0x40
#define XSPI_CR_MANUAL_SSELECT	0x80
#define XSPI_CR_TRANS_INHIBIT	0x100
#define XSPI_CR_LSB_FIRST	0x200

#define XSPI_SR_OFFSET		0x64	/* Status Register */

#define XSPI_SR_RX_EMPTY_MASK	0x01	/* Receive FIFO is empty */
#define XSPI_SR_RX_FULL_MASK	0x02	/* Receive FIFO is full */
#define XSPI_SR_TX_EMPTY_MASK	0x04	/* Transmit FIFO is empty */
#define XSPI_SR_TX_FULL_MASK	0x08	/* Transmit FIFO is full */
#define XSPI_SR_MODE_FAULT_MASK	0x10	/* Mode fault error */

#define XSPI_TXD_OFFSET		0x68	/* Data Transmit Register */
#define XSPI_RXD_OFFSET		0x6c	/* Data Receive Register */

#define XSPI_SSR_OFFSET		0x70	/* 32-bit Slave Select Register */
#define XSPI_IPISR_OFFSET	0x20

/* Register definitions as per "OPB IPIF (v3.01c) Product Specification", DS414
 * IPIF registers are 32 bit
 */
#define XIPIF_V123B_DGIER_OFFSET	0x1c	/* IPIF global int enable reg */
#define XIPIF_V123B_GINTR_ENABLE	0x80000000

#define XIPIF_V123B_IISR_OFFSET		0x20	/* IPIF interrupt status reg */
#define XIPIF_V123B_IIER_OFFSET		0x28	/* IPIF interrupt enable reg */

#define XSPI_INTR_MODE_FAULT		0x01	/* Mode fault error */
#define XSPI_INTR_SLAVE_MODE_FAULT	0x02	/* Selected as slave while
						 * disabled */
#define XSPI_INTR_TX_EMPTY		0x04	/* TxFIFO is empty */
#define XSPI_INTR_TX_UNDERRUN		0x08	/* TxFIFO was underrun */
#define XSPI_INTR_RX_FULL		0x10	/* RxFIFO is full */
#define XSPI_INTR_RX_OVERRUN		0x20	/* RxFIFO was overrun */
#define XSPI_INTR_TX_HALF_EMPTY		0x40	/* TxFIFO is half empty */

#define XIPIF_V123B_RESETR_OFFSET	0x40	/* IPIF reset register */
#define XIPIF_V123B_RESET_MASK		0x0a	/* the value to write */

/* Number of bits per word */
#define XSPI_ONE_BITS_PER_WORD 1
#define XSPI_TWO_BITS_PER_WORD 2
#define XSPI_FOUR_BITS_PER_WORD 4

/* Number of data lines used to receive */
#define XSPI_RX_ONE_WIRE	1
#define XSPI_RX_FOUR_WIRE	4

/* Auto suspend timeout in milliseconds */
#define SPI_AUTOSUSPEND_TIMEOUT		3000

/* Command used for Dummy read Id */
#define SPI_READ_ID		0x9F
/* spi command */
#define SPI_PAGE_PROGRAM	0x2
#define SPI_READ			0x3
#define SPI_WRITE_DIS		0x4
#define SPI_READ_STATUS		0x5
#define SPI_WRITE_EN		0x6
#define SPI_4K_SUBSECTOR_ERASE	0x20	//4k
#define SPI_32K_SUBSECTOR_ERASE	0x52	//32k
#define SPI_SECTOR_ERASE	0xd8	//32k*2
#define SPI_DIE_ERASE		0xc4	//64K*512

#define size_4k			0x01000
#define size_32k		0x08000
#define size_64k		0x10000

/**
 * struct xilinx_spi - This definition define spi driver instance
 * @regs:		virt. address of the control registers
 * @irq:		IRQ number
 * @axi_clk:		Pointer to the AXI clock
 * @axi4_clk:		Pointer to the AXI4 clock
 * @spi_clk:		Pointer to the SPI clock
 * @dev:		Pointer to the device
 * @rx_ptr:		Pointer to the RX buffer
 * @tx_ptr:		Pointer to the TX buffer
 * @bytes_per_word:	Number of bytes in a word
 * @buffer_size:	Buffer size in words
 * @cs_inactive:	Level of the CS pins when inactive
 * @read_fn:		For reading data from SPI registers
 * @write_fn:		For writing data to SPI registers
 * @bytes_to_transfer:	Number of bytes left to transfer
 * @bytes_to_receive:	Number of bytes left to receive
 * @rx_bus_width:	Number of wires used to receive data
 * @tx_fifo:		For writing data to fifo
 * @rx_fifo:		For reading data from fifo
 */
struct xilinx_spi {
	void __iomem	*regs;	/* virt. address of the control registers */

	int		irq;
#if 0
	struct clk *axi_clk;
	struct clk *axi4_clk;
	struct clk *spi_clk;
#endif
	struct device *dev;
	u8 *rx_ptr;		/* pointer in the Tx buffer */
	const u8 *tx_ptr;	/* pointer in the Rx buffer */
	u8 bytes_per_word;
	int buffer_size;	/* buffer size in words */
	u32 cs_inactive;	/* Level of the CS pins when inactive*/
	unsigned int (*read_fn)(void __iomem *);
	void (*write_fn)(u32, void __iomem *);
	u32 bytes_to_transfer;
	u32 bytes_to_receive;
	u32 rx_bus_width;
	void (*tx_fifo)(struct xilinx_spi *xqspi);
	void (*rx_fifo)(struct xilinx_spi *xqspi);
};

/* spi part end */

struct fpga_dev {

	void __iomem *i2c_gpio_mux_base;
	void __iomem *axi_gpio_base[AXI_GPIO_DEV];
	void __iomem *axi_bram_base;
	void __iomem *axi_spi_base;

	struct xiic_i2c i2c[MAX_IIC_DEV];
	struct mutex	gpio_lock;
	u8 pages[MAX_IIC_DEV];
	u8 recover[MAX_IIC_DEV]; // store recover page packet status. 0 : not send yet, 1: send and wait result, 2: success and return back
	struct spi_master *master;
	int  (*fpga_sff_prs_get)(struct fpga_dev *dev, unsigned long *bitmap);
	int  (*fpga_sff_intr_get)(struct fpga_dev *dev, unsigned long *bitmap);
	int  (*fpga_sff_lpmode_get)(struct fpga_dev *dev, unsigned long *bitmap);
	int  (*fpga_sff_lpmode_set)(struct fpga_dev *dev, unsigned long bitmap);
	int  (*fpga_sff_reset_get)(struct fpga_dev *dev, unsigned long *bitmap);
	int  (*fpga_sff_reset_set)(struct fpga_dev *dev, unsigned long bitmap);
	int  (*fpga_sff_modsel_get)(struct fpga_dev *dev, unsigned long *bitmap);
	int  (*fpga_sff_modsel_set)(struct fpga_dev *dev, unsigned long bitmap);
	int  (*fpga_sff_polling_status_get)(struct fpga_dev *dev, int port);
	int  (*fpga_sff_polling_set)(struct fpga_dev *dev, int port, bool en);
	int  (*fpga_sff_polling_get)(struct fpga_dev *dev, int port);
	int  (*fpga_gpio_isr_set)(struct fpga_dev *dev, unsigned long val);
	int  (*fpga_gpio_isr_get)(struct fpga_dev *dev);
	int  (*fpga_gpio_ier_set)(struct fpga_dev *dev, bool en);
	int  (*fpga_gpio_ier_get)(struct fpga_dev *dev);
	int  (*fpga_gpio_gier_set)(struct fpga_dev *dev, bool en);
	int  (*fpga_gpio_gier_get)(struct fpga_dev *dev);
	int  (*fpga_gpio_intr_cb)(void);

	struct msix_entry msix_entries[MAX_MSIX_ENTRIES];
	short fpga_eeprom_page[MAX_IIC_DEV][BRAM_PAGE_CAPACITY];
};

