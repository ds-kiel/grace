#include <radio.h>
#include <stdio.h>
#include <glib.h>
#include <cc1101-spidev/cc1101.h>
#include <inttypes.h>

// returns the nanosecond timestamp at which the pulse was transmitted
static guint64 share_local_time()
{
  printf("share_local_time() -> I am not implemented yet!");
  return 0;
}

static gpointer radio_master_thread_func(gpointer data) {
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
        __u8 write_buf[4] = {0x03, 0x00, 0x00, 0x00};
        cc1101_write_tx_fifo(write_buf, 4);
      }
    }

    if (IS_STATE(cc1101_get_chip_state(), IDLE)) {
      printf("Chip is IDLING, setting back to tx\n");
      cc1101_set_transmit();
    }

    g_usleep(1e6);
  }
}

int radio_master_init() {
  int handle; // handle system not implemented, use structure instead for one session?
  handle = cc1101_init("/dev/spidev0.1");

  if (handle < 0) return 0;

  while(IS_STATE(cc1101_get_chip_state(), SETTLING)) {
    printf("device is settling\n");
  }

  cc1101_read_config(PKTCTRL1);
  cc1101_write_config(PKTCTRL1, 0x0C);
  cc1101_read_config(PKTCTRL1);

  cc1101_set_base_freq(1091553);// frequency increment for ~433MHz transmission

  // enable frequency synthesizer auto-calibration upon entering rx or tx state from idle
  int mcsm0_orig;
  mcsm0_orig = cc1101_read_config(MCSM0);
  cc1101_write_config(MCSM0, mcsm0_orig | 0x10);
  cc1101_read_config(MCSM0);

  cc1101_command_strobe(header_command_sftx);
  cc1101_set_transmit();

  g_thread_new("radio_master", radio_master_thread_func, NULL);
}
