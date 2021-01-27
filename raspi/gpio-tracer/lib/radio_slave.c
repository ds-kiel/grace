#include <radio-slave.h>
#include <stdio.h>
#include <glib.h>
#include <types.h>

#include <cc1101-spidev/cc1101.h>
#include <inttypes.h>
#include <time.h>

static GAsyncQueue *_timestamp_unref_queue; // holds local timestamps from logic analyzer
static GAsyncQueue *_timestamp_ref_queue; // holds pairs of local timestamps and reference timestamp

static gpointer radio_slave_thread_func(gpointer data) {
  while(1) {
    int ret;
    int bytes_avail;
    timestamp_t *unref_timestamp;
    timestamp_pair_t *ref_timestamp_pair;

    printf("waiting for signal on logic analyzer\n");
    unref_timestamp = g_async_queue_pop(_timestamp_unref_queue);

    // after reception radio moves to IDLE. We can't just stay in RX_MODE all the time because we might receive garbage data
    while(IS_STATE(cc1101_get_chip_state(), RX_MODE)) {
      printf("waiting for complete package reception\n");
      g_usleep(1000);
    }

    // in case of high background noise the PQT might be too small and noise is mistaking as valid data.
    // This should be the only case where the FIFO overflows so we just discard all received data.
    if (IS_STATE(cc1101_get_chip_state(), RXFIFO_OVERFLOW)) {
      cc1101_command_strobe(header_command_sfrx);
      cc1101_set_receive();
    } else if (IS_STATE(cc1101_get_chip_state(), IDLE)) {
      bytes_avail = cc1101_rx_fifo_bytes();
      if(bytes_avail == sizeof(timestamp_t) + 3) { // our packets will always have 11 bytes. Together with receiver CRC this should ensure we only process valid packets
        __u8 read_buf[bytes_avail]; // TODO module should hide kernel types?
        cc1101_read_rx_fifo(read_buf, bytes_avail);
        // read ref timestamp from read_buf.
        ref_timestamp_pair = malloc(sizeof(timestamp_pair_t));
        ref_timestamp_pair->local_timestamp_ns = *unref_timestamp;
        memcpy(&ref_timestamp_pair->reference_timestamp_ns, read_buf+1, sizeof(timestamp_t));
        g_async_queue_push(_timestamp_ref_queue, ref_timestamp_pair);

        free(unref_timestamp); // data moved to wrapper structure so we can free it now
      }
      cc1101_command_strobe(header_command_sfrx);
      cc1101_set_receive(); // we are ready to receive another package
    }
  }
}

int radio_slave_init(GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue) {
  int handle; // handle system not implemented, use structure instead for one session?

  _timestamp_unref_queue  = timestamp_unref_queue;
  _timestamp_ref_queue  = timestamp_ref_queue;

  handle = cc1101_init("/dev/spidev0.0");

  if (handle < 0) return 0;

  while(IS_STATE(cc1101_get_chip_state(), SETTLING)) {
    printf("device is settling\n");
  }

  cc1101_read_config(PKTCTRL1);
  cc1101_write_config(PKTCTRL1, 0x0C | 0x40); // 0x0C -> auto CRC Flush, 0x40 -> PQT = 2*4 = 8
  cc1101_read_config(PKTCTRL1);

  // enable frequency synthesizer auto-calibration upon entering rx or tx state from idle
  int mcsm0_orig;
  mcsm0_orig = cc1101_read_config(MCSM0);
  cc1101_write_config(MCSM0, mcsm0_orig | 0x10);
  cc1101_read_config(MCSM0);

  // frequency increment for roughly 433MHz transmission
  cc1101_set_base_freq(1091553);

  /* int mcsm1_orig; */
  /* mcsm1_orig = cc1101_read_config(MCSM1); */
  /* cc1101_write_config(MCSM1, mcsm1_orig | 0x0C); // stay in RX after receiving a packet */
  /* cc1101_read_config(MCSM1); */

  // set GDO0 GPIO function to assert transmission of sync packet
  cc1101_write_config(IOCFG0, 0x06);

  // ensure device is in IDLE mode
  cc1101_command_strobe(header_command_sidle);
}

void radio_slave_start_reception() {
  cc1101_command_strobe(header_command_sfrx);
  cc1101_set_receive();

  while(IS_STATE(cc1101_get_chip_state(), CALIBRATE)) {
    printf("device is calibrating\n");
  }

  g_thread_new("radio_slave", radio_slave_thread_func, NULL);
}
