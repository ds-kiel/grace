#ifndef CC1101_H
#define CC1101_H

#include <stddef.h>

// Master configuration

// Used to create header byte for a single transaction over the SPI interface RW e.g, for message
// queue this means access to TX/RX FIFO
#define TRANSACTION(RW, BURST, ADDR) ((RW << 7) | (BURST << 6) | ADDR | 0)

#define WRITE 0
#define READ 1

#define SINGLE 0
#define BURST 1

// Chip status byte
#define STATE_BITS 0x70
#define ADDR_BITS  0x3F
#define FIFO_BITS  0x03

// Chip status byte states (more states readable from status registers)
#define IDLE             0x00
#define RX_MODE          0x10
#define TX_MODE          0x20
#define FSTXON           0x30
#define CALIBRATE        0x40
#define SETTLING         0x50
#define RXFIFO_OVERFLOW  0x60
#define TXFIFO_UNDERFLOW 0x70
#define CHIP_RDYn        0x80

#define IS_STATE(VALUE, STATE)  (VALUE & STATE_BITS) == STATE
#define IS_RDY(VALUE, STATE)   !(VALUE & CHIP_RDY)

// command strobes
#define SRES    0x30 // reset chip
#define SFSTXON 0x31 // enable calibrate frequency synthesizer
#define SXOFF   0x32 // enable calibrate frequency synthesizer
#define SCAL    0x33
#define SRX     0x34
#define STX     0x35
#define SIDLE   0x36
#define SWOR    0x38
#define SPWD    0x39
#define SFRX    0x3A
#define SFTX    0x3B
#define SWORRST 0x3C
#define SNOP    0x3D

// FIFO access headers
#define header_rx_fifo       TRANSACTION(READ, SINGLE, 0x3F)
#define header_burst_rx_fifo TRANSACTION(READ, BURST, 0x3F)

#define header_tx_fifo       TRANSACTION(WRITE, SINGLE, 0x3F)
#define header_burst_tx_fifo TRANSACTION(WRITE, BURST, 0x3F)

// command strobes
#define header_command_sres     TRANSACTION(WRITE|READ, SINGLE, SRES    )
#define header_command_sfstxon  TRANSACTION(WRITE|READ, SINGLE, SFSTXON )
#define header_command_sxoff    TRANSACTION(WRITE|READ, SINGLE, SXOFF   )
#define header_command_scal     TRANSACTION(WRITE|READ, SINGLE, SCAL    )
#define header_command_srx      TRANSACTION(WRITE|READ, SINGLE, SRX     )
#define header_command_stx      TRANSACTION(WRITE|READ, SINGLE, STX     )
#define header_command_sidle    TRANSACTION(WRITE|READ, SINGLE, SIDLE   )
#define header_command_swor     TRANSACTION(WRITE|READ, SINGLE, SWOR    )
#define header_command_spwd     TRANSACTION(WRITE|READ, SINGLE, SPWD    )
#define header_command_sfrx     TRANSACTION(WRITE|READ, SINGLE, SFRX    )
#define header_command_sftx     TRANSACTION(WRITE|READ, SINGLE, SFTX    )
#define header_command_sworrst  TRANSACTION(WRITE|READ, SINGLE, SWORRST )
#define header_command_snop     TRANSACTION(WRITE|READ, SINGLE, SNOP    )

// status registers
#define header_status_partnum        TRANSACTION(READ, BURST, 0x30)
#define header_status_version        TRANSACTION(READ, BURST, 0x31)
#define header_status_freqest        TRANSACTION(READ, BURST, 0x32)
#define header_status_lqi            TRANSACTION(READ, BURST, 0x33)
#define header_status_rssi           TRANSACTION(READ, BURST, 0x34)
#define header_status_marcstate      TRANSACTION(READ, BURST, 0x35)
#define header_status_wortime1       TRANSACTION(READ, BURST, 0x36)
#define header_status_wortime0       TRANSACTION(READ, BURST, 0x37)
#define header_status_pktstatus      TRANSACTION(READ, BURST, 0x38)
#define header_status_vco_vc_dac     TRANSACTION(READ, BURST, 0x39)
#define header_status_txbytes        TRANSACTION(READ, BURST, 0x3A)
#define header_status_rxbytes        TRANSACTION(READ, BURST, 0x3B)
#define header_status_rcctrl1_status TRANSACTION(READ, BURST, 0x3C)
#define header_status_rcctrl0_status TRANSACTION(READ, BURST, 0x3D)

#define PRETTY_STATE(read) STATES[(read & STATE_BITS) >> 4]
#define PRETTY_FIFO_SPACE(read) read & 0x0F
#define PRETTY_STROBE(read) STROBES[(read & ADDR_BITS) - 0x30]
#define PRETTY_STATUS(read) STATUS[(read  & ADDR_BITS) - 0x30]

// bitmasks for configuration register options

// IOCFG2
#define BM_GDO2_INV(b)              (b << 6)
#define BM_GDO2_CFG(b)              (b << 0)

// IOCFG1
#define BM_GDO_DS(b)                (b << 7)
#define BM_GDO1_INV(b)              (b << 6)
#define BM_GDO1_CFG(b)              (b << 0)

// IOCFG0
#define BM_TEMP_SENSOR_ENABLE(b)    (b << 7)
#define BM_GDO0_INV(b)              (b << 6)
#define BM_GDO0_CFG(b)              (b << 0)

// FIFOTHR
#define BM_ADC_RETENTION(b)         (b << 6)
#define BM_CLOSE_IN_RX(b)           (b << 4)
#define BM_FIFO_THR(b)              (b << 0)

// SYNC1, SYNC0
#define BM_SYNC(b)                  (b << 0)

// PKTLEN
#define BM_PACKET_LENGTH(b)         (b << 0)

// PKTCTRL1
#define BM_PQT(b)                   (b << 5)
#define BM_CRC_AUTOFLUSH(b)         (b << 3)
#define BM_APPEND_STATUS(b)         (b << 2)
#define BM_ADR_CHK(b)               (b << 0)

// PKTCTRL0
#define BM_WHITE_DATA(b)            (b << 6)
#define BM_PKT_FORMAT(b)            (b << 4)
#define BM_CRC_EN(b)                (b << 2)
#define BM_LENGTH_CONFIG(b)         (b << 0)

// ADDR
#define BM_DEVICE_ADDR(b)           (b << 0)

// CHANNR
#define BM_CHAN(b)                  (b << 0)

// FSCTRL1
#define BM_FREQ_IF(b)               (b << 0)

// FSCTRL0
#define BM_FREQOFF(b)               (b << 0)

// FREQ0, FREQ1, FREQ2
#define BM_FREQ(b)                  (b << 0)

// MDMCFG4di
#define BM_CHANBW_E(b)              (b << 6)
#define BM_CHANBW_M(b)              (b << 4)
#define BM_DRATE_E(b)               (b << 0)

// MDMCFG3
#define BM_DRATE_M(b)               (b << 0)

// MDFMCFG2
#define BM_DEM_DCFILT_OFF(b)        (b << 7)
#define BM_MOD_FORMAT(b)            (b << 4)
#define BM_MANCHESTER_EN(b)         (b << 3)
#define BM_SYNC_MODE(b)             (b << 0)

// MDMCFG1
#define BM_FEC_EN(b)                (b << 7)
#define BM_NUM_PREAMBLE(b)          (b << 4)
#define BM_CHANSPC_E(b)             (b << 0)

// MDMCFG0
#define BM_CHANSPC_M(b)             (b << 0)

// DEVIATN
#define BM_DEVIATION_E(b)           (b << 4)
#define BM_DEVIATION_M(b)           (b << 0)

// MCSM2
#define BM_RX_TIME_RSSI(b)          (b << 4)
#define BM_RX_TIME_QUAL(b)          (b << 3)
#define BM_RX_TIME(b)               (b << 0)

// MCSM1
#define BM_CCA_MODE(b)              (b << 4)
#define BM_RXOFF_MODE(b)            (b << 2)
#define BM_TXOFF_MODE(b)            (b << 0)

// MCSM0
#define BM_FS_AUTOCAL(b)            (b << 4)
#define BM_PO_TIMEOUT(b)            (b << 2)
#define BM_PIN_CTRL_EN(b)           (b << 1)
#define BM_XOSC_FORCE_ON(b)         (b << 0)

// FOCCFG
#define BM_FOC_BS_CS_GATE(b)        (b << 5)
#define BM_FOC_PRE_K(b)             (b << 3)
#define BM_FOC_POST_K(b)            (b << 2)
#define BM_FOC_LIMIT(b)             (b << 0)

// BSCFG
#define BM_BS_PRE_KI(b)             (b << 6)
#define BM_BS_PRE_KP(b)             (b << 4)
#define BM_BS_POST_KI(b)            (b << 3)
#define BM_BS_POST_KP(b)            (b << 2)
#define BM_BS_LIMIT(b)              (b << 0)

// AGCCTRL2
#define BM_MAX_DVGA_GAIN(b)         (b << 6)
#define BM_MAX_LNA_GAIN(b)          (b << 3)
#define BM_MAGN_TARGET(b)           (b << 0)

// AGCCTRL1
#define BM_AGC_LNA_PRIORITY(b)      (b << 6)
#define BM_CARRIER_SENSE_REL_THR(b) (b << 4)
#define BM_CARRIER_SENSE_ABS_THR(b) (b << 0)

// AGCCTRL0
#define BM_HYST_LEVEL(b)            (b << 6)
#define BM_WAIT_TIME(b)             (b << 4)
#define BM_AGC_FREEZE(b)            (b << 2)
#define BM_FILTER_LENGTH(b)         (b << 0)

// WOREVT1, WOREVT0
#define BM_EVENT0(b)                (b << 0)

// WORCTRL
#define BM_RC_PD(b)                 (b << 7)
#define BM_EVENT1(b)                (b << 4)
#define BM_RC_CAL(b)                (b << 3)
#define BM_WOR_RES(b)               (b << 0)

// FREND1
#define BM_LNA_CURRENT(b)           (b << 6)
#define BM_LNA2MIX_CURRENT(b)       (b << 4)
#define BM_LODIV_BUF_CURRENT_RX(b)  (b << 2)
#define BM_MIX_CURRENT(b)           (b << 0)

// FREND0
#define BM_LODIV_BUF_CURRENT_TX(b)  (b << 4)
#define BM_PA_POWER(b)              (b << 0)

// FSCAL3
#define BM_FSCAL36(b)               (b << 6) // For some reason these  ...
#define BM_CHP_CURR_CAL_EN(b)       (b << 4)
#define BM_FSCAL30(b)               (b << 0) // ... are called the same in the cc1101 docs

// FSCAL2
#define BM_VCO_CORE_H_EN(b)         (b << 5)
#define BM_FSCAL2(b)                (b << 0)

// FSCAL1
#define BM_FSCAL1(b)                (b << 0)

// FSCAL0
#define BM_FSCAL0(b)                (b << 0)

// RCCTRL1
#define BM_RCCTRL1(b)               (b << 0)

// RCCTRL0
#define BM_RCCTRL0(b)               (b << 0)

// FSTEST
#define BM_FSTEST(b)                (b << 0)

// PTEST
#define BM_PTEST(b)                 (b << 0)

// AGCTEST
#define BM_AGCTEST(b)               (b << 0)

// TEST2
#define BM_TEST2(b)                 (b << 0)

// TEST1
#define BM_TEST1(b)                 (b << 0)

// TEST0x
#define BM_TEST02(b)                (b << 2) // these ...
#define BM_VCO_SEL_CAL_EN(b)        (b << 1)
#define BM_TEST00(b)                (b << 0) // ... are also called the same in the docs

// configuration registers
enum cc1101_conf_regs {
  IOCFG2   = 0x00,
  IOCFG1   = 0x01,
  IOCFG0   = 0x02,
  FIFOTHR  = 0x03,
  SYNC1    = 0x04,
  SYNC0    = 0x05,
  PKTLEN   = 0x06,
  PKTCTRL1 = 0x07,
  PKTCTRL0 = 0x08,
  ADDR     = 0x09,
  CHANNR   = 0x0A,
  FSCTRL1  = 0x0B,
  FSCTRL0  = 0x0C,
  FREQ2    = 0x0D,
  FREQ1    = 0x0E,
  FREQ0    = 0x0F,
  MDMCFG4  = 0x10,
  MDMCFG3  = 0x11,
  MDMCFG2  = 0x12,
  MDMCFG1  = 0x13,
  MDMCFG0  = 0x14,
  DEVIATN  = 0x15,
  MCSM2    = 0x16,
  MCSM1    = 0x17,
  MCSM0    = 0x18,
  FOCCFG   = 0x19,
  BSCFG    = 0x1A,
  AGCCTRL2 = 0x1B,
  AGCCTRL1 = 0x1C,
  AGCCTRL0 = 0x1D,
  WOREVT1  = 0x1E,
  WOREVT0  = 0x1F,
  WORCTRL  = 0x20,
  FREND1   = 0x21,
  FREND0   = 0x22,
  FSCAL3   = 0x23,
  FSCAL2   = 0x24,
  FSCAL1   = 0x25,
  FSCAL0   = 0x26,
  RCCTRL1  = 0x27,
  RCCTRL0  = 0x28,
  FSTEST   = 0x29,
  PTEST    = 0x2A,
  AGCTEST  = 0x2B,
  TEST2    = 0x2C,
  TEST1    = 0x2D,
  TEST0    = 0x2E,
};

typedef unsigned char cc1101_configuration_t[TEST0 + 1];

extern cc1101_configuration_t cc1101_reset_config;

void cc1101_generate_reset_configuration(cc1101_configuration_t config);

// maximizes the sensitivy of an existing configuration `config`
void cc1101_high_sense_configuration(cc1101_configuration_t config);

// maximizes the data rate of an existing configuration `config`
void cc1101_high_drate_configuration(cc1101_configuration_t config);

void cc1101_write_configuration(cc1101_configuration_t config);
int cc1101_check_configuration_values(cc1101_configuration_t config);

static void spi_access(unsigned char* data, int len, unsigned char *read);

// TODO some kind of cc1101 management structure? Instead of singleton
int  cc1101_init(char* spi_device);
void cc1101_deinit();
unsigned char cc1101_command_strobe(unsigned char strobe);
unsigned char cc1101_read_status_reg(unsigned char header);
unsigned char cc1101_get_chip_state();
char* cc1101_get_chip_state_str();
unsigned char cc1101_rx_fifo_bytes();
unsigned char cc1101_tx_fifo_bytes();
void cc1101_set_base_freq(cc1101_configuration_t config, int increment);
void cc1101_write_config(unsigned char config, unsigned char value);
unsigned char cc1101_read_config(unsigned char config);
int  cc1101_read_rx_fifo(unsigned char *read, size_t len);
void cc1101_write_tx_fifo(unsigned char *data, size_t len);
void cc1101_set_receive(); // TODO add a option to stay receiving permanently
void cc1101_set_transmit();
void cc1101_transmit(unsigned char* data, size_t len);

#endif /* CC1101_H */
