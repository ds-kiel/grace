#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cc1101.h"

int main(int argc, char *argv[])
{
  int handle; // handle system not implemented, use structure instead for one session?
  handle = cc1101_init("/dev/spidev0.0");

  if (handle < 0) return 0;
  /* __u8 testdata[4] = {0x74, 0x38, 0x62, 0x11}; */

  while(IS_STATE(cc1101_get_chip_state(), SETTLING)) {
    printf("device is settling\n");
  }

  cc1101_set_base_freq(1091553);// frequency increment for roughly 433MHz transmission

  unsigned char PKTCTRL1_old,PKTCTRL1_new;
  PKTCTRL1_old = cc1101_read_config(PKTCTRL1);
  PKTCTRL1_new =
    1 << 5 | // PQT (preamble quality estimator threshold)
    0 << 4 | // Not used
    1 << 3 | // CRC_AUTOFLUSH
    1 << 2 | // APPEND_STATUS
    0 << 0;  // ADR_CHK

  cc1101_write_config(PKTCTRL1, PKTCTRL1_new);
  cc1101_read_config(PKTCTRL1);

  // enable frequency synthesizer auto-calibration upon entering rx or tx state from idle every third time
  unsigned char MCSM0_old, MCSM0_new;
  MCSM0_new =
    0 << 6 | // Not used
    2 << 4 | // FS_AUTOCAL
    1 << 2 | // PO_TIMEOUT
    0 << 1 | // PIN_CTRL_EN
    0 << 0;  // XOSC_FORCE_ON

  MCSM0_old = cc1101_read_config(MCSM0);
  cc1101_write_config(MCSM0, MCSM0_new);
  cc1101_read_config(MCSM0);



  // set level of hysteresis
  unsigned char AGCCTRL0_old, AGCCTRL0_new;
  AGCCTRL0_old = cc1101_read_config(AGCCTRL0);

  AGCCTRL0_new =
    2 << 6 | // HYST_LEVEL
    3 << 4 | // WAIT_TIME
    0 << 2 | // AGC_FREEZE
    3 << 0;  // FILTER_LENGTH

  cc1101_write_config(AGCCTRL0, AGCCTRL0_new);
  cc1101_read_config(AGCCTRL0);

  // set relative carrier sense threshold to 6dB
  unsigned char AGCCTRL1_old, AGCCTRL1_new;
  AGCCTRL1_old = cc1101_read_config(AGCCTRL1);

  AGCCTRL1_new =
    1 << 6 | // AGC_LNA_PRIORITY (1: default)
    /* 0 << 6 | // AGC_LNA_PRIORITY (0: decrease LNA 2 gain to mininum before decreasing LNA gain) */
    0 << 4 | // CARRIER_SENSE_REL_THR (0: default)
    0 << 0;   // CARRIER_SENSE_ABS_THR (0: default)

  /* cc1101_write_config(AGCCTRL1, AGCCTRL1_old | 0x20); */
  cc1101_write_config(AGCCTRL1, AGCCTRL1_new);
  cc1101_read_config(AGCCTRL1);

  printf("Set AGCCTRL to %x\n", AGCCTRL1_new);

  // set MAGN_TARGET to highest setting
  unsigned char AGCCTRL2_old, AGCCTRL2_new;
  AGCCTRL2_new =
    0 << 6 | // MAX_DVGA_GAIN
    0 << 3 | // MAX_LNA_GAIN
    6 << 0;  // MAGN_TARGET

  AGCCTRL2_old = cc1101_read_config(AGCCTRL2);
  cc1101_write_config(AGCCTRL2, AGCCTRL2_new);
  cc1101_read_config(AGCCTRL2);

  // 0x04 require both 16/16 sync word recognition; 0x06 additionally carrier sense above threshold
  unsigned char MDMCFG2_old, MDMCFG2_new;
  MDMCFG2_old = cc1101_read_config(MDMCFG2);
  MDMCFG2_new =
    0 << 7 | // DEM_DCFILT_OFF
    0 << 4 | // MOD_FORMAT
    0 << 3 | // MANCHESTER_EN
    2 << 0;  // SYNC_MODE
  /* cc1101_write_config(MDMCFG2, MDMCFG2_old | 0x02); */
  cc1101_write_config(MDMCFG2, MDMCFG2_new);
  cc1101_read_config(MDMCFG2);

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

  unsigned char IOCFG0_old, IOCFG0_new;
  IOCFG0_old = cc1101_read_config(IOCFG0);
  IOCFG0_new =
    0 << 7 | // TEMP_SENSOR_ENABLE
    0 << 6 | // GDO0_INV
    7 << 0;  // GDO0_CFG
  cc1101_write_config(IOCFG0, IOCFG0_new);

  cc1101_command_strobe(header_command_scal);

  while(IS_STATE(cc1101_get_chip_state(), CALIBRATE)) {
    printf("device is calibrating. wait.\n");
  }

  cc1101_set_receive();

  while(1) {
    int ret;
    int bytes_avail;


    while(IS_STATE(cc1101_get_chip_state(), CALIBRATE)) {
      printf("device is calibrating. wait.\n");
    }

    if (IS_STATE(cc1101_get_chip_state(), RXFIFO_OVERFLOW)) {
      cc1101_command_strobe(header_command_sfrx);
    }

    bytes_avail = cc1101_rx_fifo_bytes();

    if(bytes_avail > 0) {
      unsigned char read_buf[bytes_avail]; // TODO module should hide kernel types?
      cc1101_read_rx_fifo(read_buf, bytes_avail);

      printf("received %d bytes of data\n", bytes_avail);
    }

    if (IS_STATE(cc1101_get_chip_state(), IDLE)) {
      cc1101_set_receive();
    }

    usleep(500e3);
    /* sleep(5); */
    /* printf("Reading from fifo\n"); */

    /* ret = cc1101_read_rx_fifo(read_buf); */
    /* if (ret > 0) { */
    /*   printf("read bytes %d\n", ret); */
    /* } */
    /* printf("Setting receiver\n"); */
    /* cc1101_set_receive(); */
  }

  /* cc1101_transmit(testdata, 4); */

  // How to access configuration register
  /* spi_write(file, TRANSACTION(READ, 0, ADDR), 0x00); */

  // get amount of bytes in tx queue
  /* spi_write(file, header_status_txbytes, 0x00); */
  cc1101_deinit();

  return 0;
}
