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

  cc1101_read_config(PKTCTRL1);
  cc1101_write_config(PKTCTRL1, 0x0C);
  cc1101_read_config(PKTCTRL1);

  cc1101_set_base_freq(1091553);// frequency increment for roughly 433MHz transmission

  // enable frequency synthesizer auto-calibration upon entering rx or tx state from idle
  int mcsm0_orig;
  mcsm0_orig = cc1101_read_config(MCSM0);
  cc1101_write_config(MCSM0, mcsm0_orig | 0x10);
  cc1101_read_config(MCSM0);

  cc1101_command_strobe(header_command_sftx);
  cc1101_set_transmit();

  while(1) {
    int ret;
    int bytes_in_fifo;

    while(IS_STATE(cc1101_get_chip_state(), CALIBRATE)) {
      printf("device is calibrating. wait.\n");
    }


    if (IS_STATE(cc1101_get_chip_state(), TXFIFO_UNDERFLOW)) {
      cc1101_command_strobe(header_command_sftx);
    }

    if (IS_STATE(cc1101_get_chip_state(), TX_MODE)) {
      bytes_in_fifo = cc1101_tx_fifo_bytes();
      if(bytes_in_fifo < 10) {
        printf("enough space. Sending bytes\n");
        unsigned char write_buf[4] = {0x03, 0x00, 0x00, 0x00};
        cc1101_write_tx_fifo(write_buf, 4);
      }
    }
    if (IS_STATE(cc1101_get_chip_state(), IDLE)) {
      printf("Chip is IDLING, setting back to tx\n");
      cc1101_set_transmit();
    }

    sleep(1);
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
