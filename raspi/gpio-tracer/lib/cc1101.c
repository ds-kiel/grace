#include <cc1101.h>

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <glib.h>
#include <linux/spi/spidev.h>

#define DEBUG_LVL_TRACE 0
#define SPI_FREQ 5000000

static const char STATES[8][20]   = {
  "IDLE"
  , "RX"
  , "TX"
  , "FSTXON"
  , "CALIBRATE"
  , "SETTLING"
  , "RXFIFO_OVERFLOW"
  , "TXFIFO_UNDERFLOW"
};

static const char STROBES[14][10] = {
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

static const char STATUS[18][14] = {
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

cc1101_configuration_t cc1101_reset_config = {
  [IOCFG2]   =
  BM_GDO2_INV(0)                 |
  BM_GDO2_CFG(0x29)

  ,[IOCFG1]   =
  BM_GDO_DS(0)                   |
  BM_GDO1_INV(0)                 |
  BM_GDO1_CFG(0x2E)

  ,[IOCFG0]   =
  BM_TEMP_SENSOR_ENABLE(0)       |
  BM_GDO0_INV(0)                 |
  BM_GDO0_CFG(0x3F)

  ,[FIFOTHR]  =
  BM_ADC_RETENTION(0)            |
  BM_CLOSE_IN_RX(0)              |
  BM_FIFO_THR(0b0111)

  ,[SYNC1]    =
  BM_SYNC(0xD3)

  ,[SYNC0]    =
  BM_SYNC(0x91)

  ,[PKTLEN]   =
  BM_PACKET_LENGTH(0xFF)

  ,[PKTCTRL1] =
  BM_PQT(0x00)                   |
  BM_CRC_AUTOFLUSH(0)            |
  BM_APPEND_STATUS(1)            |
  BM_ADR_CHK(0)

  ,[PKTCTRL0] =
  BM_WHITE_DATA(1)               |
  BM_PKT_FORMAT(0)               |
  BM_CRC_EN(1)                   |
  BM_LENGTH_CONFIG(1)

  ,[ADDR]     =
  BM_DEVICE_ADDR(0)

  ,[CHANNR]   =
  BM_CHAN(0)

  ,[FSCTRL1]  =
  BM_FREQ_IF(0x0F)

  ,[FSCTRL0]  =
  BM_FREQOFF(0x00)

  ,[FREQ2]    =
  BM_FREQ(0x1E)

  ,[FREQ1]    =
  BM_FREQ(0xC4)

  ,[FREQ0]    =
  BM_FREQ(0xEC)

  ,[MDMCFG4]  =
  BM_CHANBW_E(0x02)              |
  BM_CHANBW_M(0x00)              |
  BM_DRATE_E(0x0C)

  ,[MDMCFG3]  =
  BM_DRATE_M(0x22)

  ,[MDMCFG2]  =
  BM_DEM_DCFILT_OFF(0)           |
  BM_MOD_FORMAT(0)               |
  BM_MANCHESTER_EN(0)            |
  BM_SYNC_MODE(0b010)

  ,[MDMCFG1]  =
  BM_FEC_EN(0)                   |
  BM_NUM_PREAMBLE(0b010)         |
  BM_CHANSPC_E(0b10)

  ,[MDMCFG0]  =
  BM_CHANSPC_M(0xF8)

  ,[DEVIATN]  =
  BM_DEVIATION_E(0b100)          |
  BM_DEVIATION_M(0b111)

  ,[MCSM2]    =
  BM_RX_TIME_RSSI(0)             |
  BM_RX_TIME_QUAL(0)             |
  BM_RX_TIME(0b111)

  ,[MCSM1]    =
  BM_CCA_MODE(0b11)              |
  BM_RXOFF_MODE(0b00)            |
  BM_TXOFF_MODE(0b00)

  ,[MCSM0]    =
  BM_FS_AUTOCAL(0b00)            |
  BM_PO_TIMEOUT(0b01)            |
  BM_PIN_CTRL_EN(0)              |
  BM_XOSC_FORCE_ON(0)

  ,[FOCCFG]   =
  BM_FOC_BS_CS_GATE(1)           |
  BM_FOC_PRE_K(0b10)             |
  BM_FOC_POST_K(1)               |
  BM_FOC_LIMIT(0b10)

  ,[BSCFG]    =
  BM_BS_PRE_KI(0b01)             |
  BM_BS_PRE_KP(0b10)             |
  BM_BS_POST_KI(1)               |
  BM_BS_POST_KP(1)               |
  BM_BS_LIMIT(0b00)

  ,[AGCCTRL2] =
  BM_MAX_DVGA_GAIN(0b00)         |
  BM_MAX_LNA_GAIN(0b000)         |
  BM_MAGN_TARGET(0b011)

  ,[AGCCTRL1] =
  BM_AGC_LNA_PRIORITY(1)         |
  BM_CARRIER_SENSE_REL_THR(0b00) |
  BM_CARRIER_SENSE_ABS_THR(0b0000)

  ,[AGCCTRL0] =
  BM_HYST_LEVEL(0b10)            |
  BM_WAIT_TIME(0b01)             |
  BM_AGC_FREEZE(0b00)            |
  BM_FILTER_LENGTH(0b01)

  ,[WOREVT1]  =
  BM_EVENT0(0x87)

  ,[WOREVT0]  =
  BM_EVENT0(0x6B)

  ,[WORCTRL]  =
  BM_RC_PD(1)                    |
  BM_EVENT1(0b111)               |
  BM_RC_CAL(1)                   |
  BM_WOR_RES(0b00)

  ,[FREND1]   =
  BM_LNA_CURRENT(0b01)           |
  BM_LNA2MIX_CURRENT(0b01)       |
  BM_LODIV_BUF_CURRENT_RX(0b01)  |
  BM_MIX_CURRENT(0b10)

  ,[FREND0]   =
  BM_LODIV_BUF_CURRENT_TX(0x01)  |
  BM_PA_POWER(0x00)

  ,[FSCAL3]   =
  BM_FSCAL36(0x02)               |
  BM_CHP_CURR_CAL_EN(0x02)       |
  BM_FSCAL30(0b1001)

  ,[FSCAL2]   =
  BM_VCO_CORE_H_EN(0)            |
  BM_FSCAL2(0x0A)

  ,[FSCAL1]   =
  BM_FSCAL1(0x20)

  ,[FSCAL0]   =
  BM_FSCAL0(0x0D)

  ,[RCCTRL1]  =
  BM_RCCTRL1(0x41)

  ,[RCCTRL0]  =
  BM_RCCTRL0(0x00)

  ,[FSTEST]   =
  BM_FSTEST(0x59)

  ,[PTEST]    =
  BM_PTEST(0x7F)

  ,[AGCTEST]  =
  BM_AGCTEST(0x3F)

  ,[TEST2]    =
  BM_TEST2(0x88)

  ,[TEST1]    =
  BM_TEST1(0x31)

  ,[TEST0]    =
  BM_TEST02(0x02)                |
  BM_VCO_SEL_CAL_EN(1)           |
  BM_TEST00(1)
};

void cc1101_generate_reset_configuration(cc1101_configuration_t config) {
  memcpy(config, cc1101_reset_config, sizeof(cc1101_configuration_t));
}

void cc1101_high_sense_configuration(cc1101_configuration_t config) {
  config[PKTCTRL1] =
    BM_PQT(2) |
    BM_CRC_AUTOFLUSH(1) |
    BM_APPEND_STATUS(1) |
    BM_ADR_CHK(0);

  config[MCSM0] =
    BM_FS_AUTOCAL(2) |
    BM_PO_TIMEOUT(1) |
    BM_PIN_CTRL_EN(0) |
    BM_XOSC_FORCE_ON(0);

  config[AGCCTRL0] =
    BM_HYST_LEVEL(2) |
    BM_WAIT_TIME(3) |
    BM_AGC_FREEZE(0) |
    BM_FILTER_LENGTH(3);

  config[AGCCTRL1] =
    BM_AGC_LNA_PRIORITY(1) |
    BM_CARRIER_SENSE_REL_THR(0) |
    BM_CARRIER_SENSE_ABS_THR(0);

  config[AGCCTRL2] =
    BM_MAX_DVGA_GAIN(0) |
    BM_MAX_LNA_GAIN(0) |
    BM_MAGN_TARGET(6);

  config[MDMCFG2] =
    BM_DEM_DCFILT_OFF(0) |
    BM_MOD_FORMAT(0) |
    BM_MANCHESTER_EN(0) |
    BM_SYNC_MODE(2);

  config[MDMCFG4] =
    BM_CHANBW_E(3) |
    BM_CHANBW_M(3) |
    BM_DRATE_E(7);

  config[MDMCFG3] =
    BM_DRATE_M(228);

  config[DEVIATN] =
    BM_DEVIATION_E(3) |
    BM_DEVIATION_M(1);

  config[IOCFG0] =
    BM_TEMP_SENSOR_ENABLE(0) |
    BM_GDO0_INV(0) |
    BM_GDO0_CFG(7);
}

void cc1101_high_drate_configuration(cc1101_configuration_t config) {
  config[PKTCTRL1] =
    BM_PQT(2) |
    BM_CRC_AUTOFLUSH(1) |
    BM_APPEND_STATUS(1) |
    BM_ADR_CHK(0);

  config[MCSM0] =
    BM_FS_AUTOCAL(2) |
    BM_PO_TIMEOUT(1) |
    BM_PIN_CTRL_EN(0) |
    BM_XOSC_FORCE_ON(0);

  config[AGCCTRL0] =
    BM_HYST_LEVEL(2) |
    BM_WAIT_TIME(3) |
    BM_AGC_FREEZE(0) |
    BM_FILTER_LENGTH(3);

  config[AGCCTRL1] =
    BM_AGC_LNA_PRIORITY(1) |
    BM_CARRIER_SENSE_REL_THR(0) |
    BM_CARRIER_SENSE_ABS_THR(0);

  config[AGCCTRL2] =
    BM_MAX_DVGA_GAIN(0) |
    BM_MAX_LNA_GAIN(0) |
    BM_MAGN_TARGET(6);

  config[MDMCFG2] =
    BM_DEM_DCFILT_OFF(0) |
    BM_MOD_FORMAT(0) |
    BM_MANCHESTER_EN(0) |
    BM_SYNC_MODE(2);

  config[MDMCFG4] =
    BM_CHANBW_E(3) |
    BM_CHANBW_M(3) |
    BM_DRATE_E(7);

  config[MDMCFG3] =
    BM_DRATE_M(228);

  config[DEVIATN] =
    BM_DEVIATION_E(3) |
    BM_DEVIATION_M(1);

  config[IOCFG0] =
    BM_GDO0_INV(0) |
    BM_GDO0_CFG(7);
}


int spi_dev_file;

int cc1101_init(char* spi_device) {
  unsigned char mode, lsb, bits;
  __u32 speed;

  if ((spi_dev_file = open(spi_device, O_RDWR)) < 0 ) {
    printf("Failed to open the %s\n", spi_device);
    exit(1);
  }

  // Device defaults
  /* mode = SPI_CPOL|SPI_CS_HIGH|SPI_CPHA|0; */
  mode = SPI_CPOL|SPI_CPHA;
  if (ioctl(spi_dev_file, SPI_IOC_WR_MODE, &mode) < 0) {
    perror("Can't set spi mode");
    return -1;
  }

  lsb = 0; // Most signification bit first
  if (ioctl(spi_dev_file, SPI_IOC_WR_LSB_FIRST, &lsb) < 0) {
    perror("Can't set spi lsb");
    return -1;
  }

  bits = 0;
  if (ioctl(spi_dev_file, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
    perror("Can't set spi bits per word");
    return -1;
  }

  speed = SPI_FREQ;
  if (ioctl(spi_dev_file, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
    perror("Can't set spi max speed");
    return -1;
  }

  cc1101_command_strobe(header_command_sres);
  cc1101_command_strobe(header_command_sidle);
  return 1; // return handle (If implementation with handle)
}

void cc1101_deinit() {
  cc1101_command_strobe(header_command_sres);
  close(spi_dev_file);
};

// 8 bits per word, e.g. N bytes of data will result in N seperate transmissions
// TODO swap len and read
static void spi_access(unsigned char* data, int len, unsigned char *read) {
  int ret;
  unsigned char write_buf[len];
  unsigned char read_buf[len];
  struct spi_ioc_transfer transfers[len];

  memset(transfers, 0, sizeof transfers);
  memset(write_buf, 0, sizeof write_buf);
  memset(read_buf,  0, sizeof read_buf);

  memcpy(write_buf, data, len);

  /* printf("sending: "); */
  for (size_t i = 0; i < len; ++i) {
    transfers[i].len = 1; /* number of bytes to write */
    transfers[i].cs_change = 0; /* Keep CS activated */
    /* channil[0].word_delay_usecs = 1; */
    transfers[i].delay_usecs = 1;
    transfers[i].speed_hz = SPI_FREQ;
    transfers[i].bits_per_word = 8;
    transfers[i].rx_buf = (__u64) (read_buf+i);
    transfers[i].tx_buf = (__u64) (write_buf+i);
    /* printf("0x%02X ", write_buf[i]); */
  }// printf("\n");

  if (ret = ioctl(spi_dev_file, SPI_IOC_MESSAGE(len), transfers) < 0) {
    perror("SPI_IOC_MESSAGE");
    return;
  }

  memcpy(read, read_buf, len);
}

unsigned char cc1101_command_strobe(unsigned char strobe) {
  unsigned char read[2];
  unsigned char data[2] = {strobe, 0x00};
  spi_access(data, 2, read);

  return read[1];
}

// Transmit len number of bytes TODO add address
void cc1101_write_tx_fifo(unsigned char* data, size_t len) {
  /* for (size_t i = 0; i < 64; ++i) { */
    unsigned char read[len+1];
    unsigned char data_w_header[len+1];

    memcpy (data_w_header+1, data, len);
    data_w_header[0] = header_burst_tx_fifo;
    // TODO split into multiple transmission if len exceeds 64 byte

    spi_access(data_w_header, len+1, read);
}

// Return number of bytes read from FIFO or -1 if device in wrong mode
// len: amount of bytes to read from fifo.
int cc1101_read_rx_fifo(unsigned char *read, size_t len) {
  unsigned char ret;
  unsigned char fifo_bytes;
  unsigned char read_buf[len+1];
  unsigned char data[len+1];
  memset(data, 0 , len);
  data[0] = header_burst_rx_fifo;

  unsigned char cc1101_get_chip_state();
  // check FIFO for content
  ret = cc1101_command_strobe(header_command_snop);

  // TODO while(IS_STATE(ret, SETTLING)) loop?
  /* if (!(IS_STATE(ret, RX_MODE)) || IS_STATE(ret, RXFIFO_OVERFLOW)) */
  /*   return -1; */

  fifo_bytes = cc1101_rx_fifo_bytes();

  if (DEBUG_LVL_TRACE)  printf("0x%02X bytes in RX FIFO\n", fifo_bytes);
  /* if (fifo_bytes < len) */
  /*   return -1; */

  spi_access(data, len+1, read_buf);
  memcpy(read, read_buf+1, len);

  if (DEBUG_LVL_TRACE) {
    printf("read from fifo: ");
    for(size_t i = 0; i < len; i++) {
      printf("0x%02X ", read[i]);
    } printf("\n");
  }


  /* if (ret & FIFO_BITS >= 0x03) { */
  /*   fifo_bytes = cc1101_read_status_reg(header_status_rxbytes); */
  /* } */
}

//
void cc1101_set_receive() {
  unsigned char ret;
  ret = cc1101_get_chip_state();
  if (!(IS_STATE(ret, IDLE) || IS_STATE(ret, TX_MODE)))
    printf("can not set chip to RX state. Chip is neither in IDLE or TX_MODE\n");

  cc1101_command_strobe(header_command_srx);
  cc1101_get_chip_state();
}

void cc1101_set_transmit() {
  unsigned char ret;
  ret = cc1101_get_chip_state();
  if (!(IS_STATE(ret, IDLE) | IS_STATE(ret, RX_MODE)))
    printf("Could not set chip to TX state\n");

  cc1101_command_strobe(header_command_stx);
  cc1101_get_chip_state();
}

void cc1101_set_base_freq(cc1101_configuration_t config, int increment) {
  unsigned char data[3];
  if (increment > (1 << 22))
    printf("frequency increment too large!\n");

  data[0] = (increment & 0x3F0000) >> 16;
  data[1] = (increment & 0xFF00) >> 8;
  data[2] = increment & 0xFF;

  config[FREQ2] = data[0];
  config[FREQ1] = data[1];
  config[FREQ0] = data[2];
}

// read status register, use header_status_ definitions from cc1101.h
unsigned char cc1101_read_status_reg(unsigned char header) {
  unsigned char read[2];
  unsigned char data[2] = {header, 0x00};
  spi_access(data, 2, read);
  if (DEBUG_LVL_TRACE) printf("Received 0x%02X\n", read[1]);
  return read[1];
}

void cc1101_write_config(unsigned char config, unsigned char value) {
  unsigned char read[2];
  unsigned char data[2] = {TRANSACTION(WRITE, SINGLE, config), value};
  spi_access(data, 2, read);
  if (DEBUG_LVL_TRACE) printf("writing to config 0x%02X value 0x%02X\n", config, value);
}

unsigned char cc1101_read_config(unsigned char config) {
  unsigned char read[2];
  unsigned char data[2] = {TRANSACTION(READ, SINGLE, config), 0x00};
  spi_access(data, 2, read);
  if (DEBUG_LVL_TRACE) printf("reading from config 0x%02X value 0x%02X\n", config, read[1]);
  return read[1];
}

unsigned char cc1101_rx_fifo_bytes() {
  return cc1101_read_status_reg(header_status_rxbytes) & 0x7F;
}

unsigned char cc1101_tx_fifo_bytes() {
  return cc1101_read_status_reg(header_status_txbytes) & 0x7F;
}

unsigned char cc1101_get_chip_state() {
  unsigned char ret;
  ret = cc1101_command_strobe(header_command_snop);

  return ret & STATE_BITS;
}

char* cc1101_get_chip_state_str() {
  return STATES[cc1101_get_chip_state() >> 4];
}

void cc1101_write_configuration(cc1101_configuration_t config) {
  for (unsigned char reg_addr = 0x00; reg_addr <= TEST0; reg_addr++) { // TEST0 is reg with highest address
    // TODO we should save the configuration values that are currently loaded into the transceiver somewhere,
    // so we do not write values that are left unchanged by the configuration
    cc1101_write_config(reg_addr, config[reg_addr]);
  }
}


// checks whether the values in the register of the cc1101 transceiver match the values in the configuration
// in case there is a mismatch, the addr of the register with the mismatch is returned.
// Only the last mismatch is returned.
int cc1101_check_configuration_values(cc1101_configuration_t config) {
  int read;
  int mismatch = -1;

  for (unsigned char reg_addr = 0x00; reg_addr <= TEST0; reg_addr++) { // TEST0 is reg with highest address
    // TODO we should save the configuration values that are currently loaded into the transceiver somewhere,
    // so we do not write values that are left unchanged by the configuration
    if ((read = cc1101_read_config(reg_addr)) != config[reg_addr]) {
      g_message("Device: 0x%x Config: 0x%x", read, config[reg_addr]);
      mismatch = reg_addr;
    }
  }

  return mismatch;
}

