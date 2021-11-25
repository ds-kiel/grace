#include "cc1101.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spi.h"
#include "time.h"

uint8_t __time_seconds_packet_read[TIME_PACKET_LEN];

const uint8_t STATES[8][20]   = {
  "IDLE"
  , "RX"
  , "TX"
  , "FSTXON"
  , "CALIBRATE"
  , "SETTLING"
  , "RXFIFO_OVERFLOW"
  , "TXFIFO_UNDERFLOW"
};

const uint8_t STROBES[14][10] = {
  "sres"
  , "sfstxon"
  , "sxoff"
  , "scal"
  , "srx"
  , "stx"
  , "sidle"
  , ""
  ,  "swor"
  , "spwd"
  , "sfrx"
  , "sftx"
  , "sworrst"
  , "snop"
};

const uint8_t STATUS[18][14] = {
  "partnum"
  , "version"
  , "freqest"
  , "lqi"
  , "rssi"
  , "marcstate"
  , "wortime1"
  , "wortime0"
  , "pktstatus"
  , "vco_vc_dac"
  , "txbytes"
  , "rxbytes"
  , "rcctrl1_status"
  , "rcctrl0_status"
};

void cc1101_init() {
  // reset device
  cc1101_command_strobe(header_command_sres);
  cc1101_command_strobe(header_command_sidle);

  // wait for device to settle
  while(IS_STATE(cc1101_get_chip_state(), SETTLING)) {
    HAL_Delay(10U); // wait 10 milliseconds
  }

  cc1101_read_config(PKTCTRL1);
  cc1101_write_config(PKTCTRL1, 0x0C);
  cc1101_read_config(PKTCTRL1);

  cc1101_set_base_freq(1091553);// frequency increment for ~433MHz transmission

  // 14.3kHz deviation, 58kHz digital channel filter bandwith
  unsigned char MDMCFG4_old, MDMCFG4_new;
  MDMCFG4_old = cc1101_read_config(MDMCFG4);
  MDMCFG4_new =
    3 << 6 | // CHANBW_E
    3 << 4 | // CHANBW_M
    7 << 0; // DRATE_E
  /* cc1101_write_config(MDMCFG4, MDMCFG4_old | 0x02); */
  cc1101_write_config(MDMCFG4, MDMCFG4_new);
  cc1101_read_config(MDMCFG4);

  unsigned char MDMCFG3_old, MDMCFG3_new;
  MDMCFG3_old = cc1101_read_config(MDMCFG3);
  MDMCFG3_new =
    228 << 0; // DRATE_M
  /* cc1101_write_config(MDMCFG3, MDMCFG3_old | 0x02); */
  cc1101_write_config(MDMCFG3, MDMCFG3_new);
  cc1101_read_config(MDMCFG3);

  unsigned char DEVIATN_old, DEVIATN_new;
  DEVIATN_old = cc1101_read_config(DEVIATN);
  DEVIATN_new =
   0  << 7 | // Not used
   3  << 4 | // DEVIATION_E
   0  << 3 | // Not used
   1  << 0;  // DEVIATION_M
  /* cc1101_write_config(DEVIATN, DEVIATN_old | 0x02); */
  cc1101_write_config(DEVIATN, DEVIATN_new);
  cc1101_read_config(DEVIATN);

  // enable frequency synthesizer auto-calibration upon entering rx or tx state from idle
  int mcsm0_orig;
  mcsm0_orig = cc1101_read_config(MCSM0);
  cc1101_write_config(MCSM0, mcsm0_orig | 0x10);
  cc1101_read_config(MCSM0);

  // set GDO0 GPIO function to assert transmission of sync packet
  cc1101_write_config(IOCFG0, 0x06);

  cc1101_command_strobe(header_command_sftx);
  cc1101_set_transmit();

  while(IS_STATE(cc1101_get_chip_state(), CALIBRATE)) {
    HAL_Delay(1U); // wait 1 milliseconds
  }
}

void cc1101_deinit() {
  cc1101_command_strobe(header_command_sres);
};

// 8 bits per word, e.g. N bytes of data will result in N seperate transmissions
// TODO swap len and read
void spi_access(uint8_t* data, int len, uint8_t *read) {
  int ret;
  uint8_t write_buf[len];
  uint8_t read_buf[len];

  memset(write_buf, 0, sizeof write_buf);
  memset(read_buf,  0, sizeof read_buf);

  memcpy(write_buf, data, len);

  HAL_SPI_Init(&hspi1);
  for(size_t i = 0; i < len; i++) {
    if ((ret = HAL_SPI_TransmitReceive(&hspi1, write_buf+i, read_buf+i, 1, 1000)) != HAL_OK) {
      printf("SPI transmission failed");
      HAL_SPI_DeInit(&hspi1);
      return;
    }
  }
  HAL_SPI_DeInit(&hspi1);
  memcpy(read, read_buf, len);
}

uint8_t cc1101_command_strobe(uint8_t strobe) {
  uint8_t read[2];
  uint8_t data[2] = {strobe, 0x00};
  spi_access(data, 2, read);

  return read[1];
}

void cc1101_fast_transmit_current_time() {
  uint8_t ret;
  HAL_SPI_Init(&hspi1);
  if ((ret = HAL_SPI_TransmitReceive(&hspi1, __time_seconds_packet, __time_seconds_packet_read, TIME_PACKET_LEN, 6000)) != HAL_OK) {
    printf("SPI transmission failed");
    HAL_SPI_DeInit(&hspi1);
    return;
  }
  HAL_SPI_DeInit(&hspi1);
}

// Transmit len number of bytes TODO add address
void cc1101_write_tx_fifo(uint8_t* data, size_t len) {
  /* for (size_t i = 0; i < 64; ++i) { */
    uint8_t read[len+1];
    uint8_t data_w_header[len+1];

    memcpy (data_w_header+1, data, len);
    data_w_header[0] = header_burst_tx_fifo;
    // TODO split into multiple transmission if len exceeds 64 byte

    spi_access(data_w_header, len+1, read);
}

// Return number of bytes read from FIFO or -1 if device in wrong mode
// len: amount of bytes to read from fifo
int cc1101_read_rx_fifo(uint8_t *read, size_t len) {
  uint8_t ret;
  uint8_t fifo_bytes;
  uint8_t read_buf[len+1];
  uint8_t data[len+1];
  memset(data, 0 , len);
  data[0] = header_burst_rx_fifo;

  // check FIFO for content
  ret = cc1101_command_strobe(header_command_snop);

  // TODO while(IS_STATE(ret, SETTLING)) loop?
  if (!(IS_STATE(ret, RX_MODE) || IS_STATE(ret, RXFIFO_OVERFLOW)))
    return -1;

  fifo_bytes = cc1101_rx_fifo_bytes();
  /* printf("0x%02X bytes in RX FIFO\n", fifo_bytes); */
  /* if (fifo_bytes < len) */
  /*   return -1; */

  spi_access(data, len+1, read_buf);
  memcpy(read, read_buf+1, len);

  return len;
/*   printf("read from fifo: "); */
  /* for(size_t i = 0; i < len; i++) { */
/*     printf("0x%02X ", read[i]); */
/*   } printf("\n"); */
  /* if (ret & FIFO_BITS >= 0x03) { */
  /*   fifo_bytes = cc1101_read_status_reg(header_status_rxbytes); */
  /* } */
}

//
void cc1101_set_receive() {
  uint8_t ret;
  ret = cc1101_get_chip_state();
  /* if (!(IS_STATE(ret, IDLE) || IS_STATE(ret, TX_MODE))) */
/*     printf("Could not set chip to RX state\n"); */

  cc1101_command_strobe(header_command_srx);
}

void cc1101_set_transmit() { // TODO function can fail should return bool
  uint8_t ret;
  ret = cc1101_get_chip_state();
  if (!(IS_STATE(ret, IDLE) || IS_STATE(ret, RX_MODE))) {
    printf("transceiver has to be in IDLE or RX state to be put into TX mode\n");
  } else cc1101_command_strobe(header_command_stx);
}

void cc1101_set_base_freq(int increment) {
  uint8_t data[3];
  /* if (increment > (1 << 22)) */
  /* { */
/*     printf("frequency increment too large!\n"); */
  /* } */


  data[0] = (increment & 0x003F0000) >> 16;
  data[1] = (increment & 0x0000FF00) >> 8;
  data[2] = increment &  0x000000FF;

  cc1101_write_config(FREQ2, data[0]);
  cc1101_write_config(FREQ1, data[1]);
  cc1101_write_config(FREQ0, data[2]);

  // verification
  cc1101_read_config(FREQ2);
  cc1101_read_config(FREQ1);
  cc1101_read_config(FREQ0);
}

// read status register, use header_status_ definitions from cc1101.h
uint8_t cc1101_read_status_reg(uint8_t header) {
  uint8_t read[2];
  uint8_t data[2] = {header, 0x00};
/*   printf("reading status register %s\n", PRETTY_STATUS(header)); */
  spi_access(data, 2, read);
/*   printf("Received 0x%02X\n", read[1]); */
  return read[1];
}

void cc1101_write_config(uint8_t config, uint8_t value) {
  uint8_t read[2];
  uint8_t data[2] = {TRANSACTION(WRITE, SINGLE, config), value};
  spi_access(data, 2, read);
/*   printf("writing to config 0x%02X value 0x%02X\n", config, value); */
}

uint8_t cc1101_read_config(uint8_t config) {
  uint8_t read[2];
  uint8_t data[2] = {TRANSACTION(READ, SINGLE, config), 0x00};
  spi_access(data, 2, read);
/*   printf("reading from config 0x%02X value 0x%02X\n", config, read[1]); */
  return read[1];
}

uint8_t cc1101_rx_fifo_bytes() {
  return cc1101_read_status_reg(header_status_rxbytes) & 0x7F;
}

uint8_t cc1101_tx_fifo_bytes() {
  return cc1101_read_status_reg(header_status_txbytes) & 0x7F;
}

uint8_t cc1101_get_chip_state() {
  uint8_t ret;
  ret = cc1101_command_strobe(header_command_snop);
/*   printf("get_chip_state() -> %s\n", PRETTY_STATE(ret)); */

  return ret & STATE_BITS;
}
