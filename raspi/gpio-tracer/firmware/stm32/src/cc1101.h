#ifndef __CC1101_H__
#define __CC1101_H__

#include <stddef.h>
#include <stdint.h>

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

// configuration registers
#define IOCFG2   0x00
#define IOCFG1   0x01
#define IOCFG0   0x02
#define FIFOTHR  0x03
#define SYNC1    0x04
#define SYNC0    0x05
#define PKTLEN   0x06
#define PKTCTRL1 0x07
#define PKTCTRL0 0x08
#define ADDR     0x09
#define CHANNR   0x0A
#define FSCTRL1  0x0B
#define FSCTRL0  0x0C
#define FREQ2    0x0D
#define FREQ1    0x0E
#define FREQ0    0x0F
#define MDMCFG4  0x10
#define MDMCFG3  0x11
#define MDMCFG2  0x12
#define MDMCFG1  0x13
#define MDMCFG0  0x14
#define DEVIATN  0x15
#define MCSM2    0x16
#define MCSM1    0x17
#define MCSM0    0x18
#define FOCCFG   0x19
#define BSCFG    0x1A
#define AGCTRL2  0x1B
#define AGCTRL1  0x1C
#define AGCTRL0  0x1D
#define WOREVT1  0x1E
#define WOREVT0  0x1F
#define WORCTRL  0x20
#define FREND1   0x21
#define FREND0   0x22
#define FSCAL3   0x23
#define FSCAL2   0x24
#define FSCAL1   0x25
#define FSCAL0   0x26
#define RCCTRL1  0x27
#define RCCTRL0  0x28
#define FSTEST   0x29
#define PTEST    0x2A
#define AGCTEST  0x2B
#define TEST2    0x2C
#define TEST1    0x2D
#define TEST0    0x2E


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

void spi_access(uint8_t* data, int len, uint8_t *read);

// TODO some kind of cc1101 management structure? Instead of singleton
void cc1101_init();
void cc1101_deinit();
uint8_t cc1101_command_strobe(uint8_t strobe);
uint8_t cc1101_read_status_reg(uint8_t header);
uint8_t cc1101_get_chip_state();
uint8_t cc1101_rx_fifo_bytes();
uint8_t cc1101_tx_fifo_bytes();
void cc1101_set_base_freq(int increment);
void cc1101_write_config(uint8_t config, uint8_t value);
uint8_t cc1101_read_config(uint8_t config);
int  cc1101_read_rx_fifo(uint8_t *read, size_t len);
void cc1101_write_tx_fifo(uint8_t *data, size_t len);
void cc1101_set_receive(); // TODO add a option to stay receiving permanently
void cc1101_set_transmit();
void cc1101_transmit(uint8_t* data, size_t len);
void cc1101_fast_transmit_current_time();


#endif /* __CC1101_H__ */
