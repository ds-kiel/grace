#include <radio-master.h>
#include <stdio.h>
#include <glib.h>
#include <types.h>

#include <cc1101-spidev/cc1101.h>
#include <inttypes.h>
#include <time.h>

static GAsyncQueue *_timestamp_unref_queue; // holds local timestamps from logic analyzer
static GAsyncQueue *_timestamp_ref_queue; // holds pairs of local timestamps and reference timestamp

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

      while(bytes_in_fifo > 64-sizeof(timestamp_t)) {
        printf("TX FIFO does not have enough space. Waiting\n");
        bytes_in_fifo = cc1101_tx_fifo_bytes();
      }

      // Critical section susceptible to indeterministic delays induced by the os scheduler. This part could be moved inside a kernel module.
      struct timespec *local_timestamp;
      clock_gettime(CLOCK_REALTIME, local_timestamp);
      timestamp_t ref_timestamp = 1e9*local_timestamp->tv_sec + local_timestamp->tv_nsec;


      // create referenced timestamp pair for postprocess task
      // TODO ultimately this should happen after sending the packet to reduce any further
      // delay.
      timestamp_pair_t *unref_timestamp_pair = malloc(sizeof(timestamp_pair_t));
      unref_timestamp_pair->reference_timestamp_ps = ref_timestamp;

      g_async_queue_push(_timestamp_unref_queue , unref_timestamp_pair);

      // sending
      __u8 write_buf[sizeof(timestamp_t)+1]; // first byte for packet length
      write_buf[0] = sizeof(timestamp_t);
      memcpy(write_buf+1, &ref_timestamp, sizeof(timestamp_t));
      cc1101_write_tx_fifo(write_buf, sizeof(timestamp_t)+1);
      // TODO there should be another function automatically prefixing with the length

      printf("Send timestamp %" PRIu64 "\n", ref_timestamp);
    }

    if (IS_STATE(cc1101_get_chip_state(), IDLE)) {
      printf("Chip is IDLING, setting back to tx\n");
      cc1101_set_transmit();
    }

    g_usleep(MASTER_TIMESTAMP_PERIOD*1e6);
  }
}



int radio_master_init(GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue) {
  int handle; // handle system not implemented, use structure instead for one session?
  handle = cc1101_init("/dev/spidev0.0");

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

  // set GDO0 GPIO function to assert transmission of sync packet
  cc1101_write_config(IOCFG0, 0x06);

  cc1101_command_strobe(header_command_sftx);
  cc1101_set_transmit();

  _timestamp_unref_queue = timestamp_unref_queue; // unreferenced
  _timestamp_ref_queue   = timestamp_ref_queue; // referenced

  g_thread_new("radio_master", radio_master_thread_func, NULL);
}
