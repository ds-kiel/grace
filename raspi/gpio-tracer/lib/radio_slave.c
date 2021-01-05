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
    timestamp_t ref_timestamp;
    timestamp_pair_t *ref_timestamp_pair;

    unref_timestamp = g_async_queue_pop(_timestamp_unref_queue);

    while(IS_STATE(cc1101_get_chip_state(), CALIBRATE)) {
      printf("device is calibrating. wait.\n");
    }

    bytes_avail = cc1101_rx_fifo_bytes();
    if (IS_STATE(cc1101_get_chip_state(), RXFIFO_OVERFLOW)) {
      cc1101_command_strobe(header_command_sfrx);
    }

    if(bytes_avail > 0) {
      __u8 read_buf[bytes_avail]; // TODO module should hide kernel types?
      cc1101_read_rx_fifo(read_buf, bytes_avail);
    }

    // read ref timestamp from read_buf. For now 200
    ref_timestamp = 200;
    ref_timestamp_pair = malloc(sizeof(timestamp_pair_t));
    ref_timestamp_pair->local_timestamp_ns = *unref_timestamp;
    ref_timestamp_pair->reference_timestamp_ns = ref_timestamp;

    g_async_queue_push(_timestamp_ref_queue, ref_timestamp_pair);
    free(unref_timestamp);

    if (IS_STATE(cc1101_get_chip_state(), IDLE)) {
      cc1101_set_receive();
    }
  }
}

int radio_slave_init(GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue) {
  int handle; // handle system not implemented, use structure instead for one session?
  handle = cc1101_init("/dev/spidev0.1");

  if (handle < 0) return 0;

  while(IS_STATE(cc1101_get_chip_state(), SETTLING)) {
    printf("device is settling\n");
  }

  cc1101_read_config(PKTCTRL1);
  cc1101_write_config(PKTCTRL1, 0x0C | 0x20); // 0x0C -> auto CRC Flush, 0x20 -> PTQ = 2
  cc1101_read_config(PKTCTRL1);

  // enable frequency synthesizer auto-calibration upon entering rx or tx state from idle
  int mcsm0_orig;
  mcsm0_orig = cc1101_read_config(MCSM0);
  cc1101_write_config(MCSM0, mcsm0_orig | 0x10);
  cc1101_read_config(MCSM0);
  cc1101_set_base_freq(1091553);// frequency increment for roughly 433MHz transmission

  cc1101_command_strobe(header_command_sfrx);
  cc1101_set_receive();

  _timestamp_unref_queue  = timestamp_unref_queue;
  _timestamp_ref_queue  = timestamp_ref_queue;

  g_thread_new("radio_slave", radio_slave_thread_func, NULL);
}
