#include <radio.h>
#include <stdio.h>
#include <glib.h>
#include <types.h>
#include <configuration.h>

#include <cc1101-spidev/cc1101.h>
#include <inttypes.h>
#include <time.h>



static GAsyncQueue *_timestamp_ref_queue; // return queue for reference timestamps
static GAsyncQueue *_timestamp_unref_queue; // process incoming gpio signals captured by the logic analyzer

static gpointer radio_thread_func(gpointer data) {
  static uint8_t no_signal_cnt = 0;
  while(1) {
    int ret;
    int bytes_avail;
    timestamp_pair_t *reference_timestamp_pair = NULL; // add timestamp from preprocess to identify pair
    ptime_t *local_timestamp_ps = NULL;

    local_timestamp_ps = g_async_queue_timeout_pop(_timestamp_unref_queue, 2e6);
    /* local_timestamp_ps = g_async_queue_pop(_timestamp_unref_queue); */
    g_debug("pop unreferenced timestamp from queue");

    // Timeout -> check wether transceiver is in correct state
    if(local_timestamp_ps == NULL) {
      /* no_signal_cnt += 1; */

      /* if (!(no_signal_cnt % 100)) { */
      /*   no_signal_cnt = 0; */
      /*   bytes_avail = cc1101_rx_fifo_bytes(); */
      /*   if(!bytes_avail) { */
      /*     g_debug("manual recalibration"); */
      /*     cc1101_command_strobe(header_command_sidle); */
      /*     cc1101_command_strobe(header_command_scal); */
      /*     while(!IS_STATE(cc1101_get_chip_state(), IDLE)) g_usleep(10000); */
      /*     cc1101_set_receive(); */
      /*   } */
      /* } */
      /* if (IS_STATE(cc1101_get_chip_state(), RXFIFO_OVERFLOW)) { */
      /*   cc1101_command_strobe(header_command_sfrx); */
      /*   cc1101_set_receive(); */
      /* } else if (IS_STATE(cc1101_get_chip_state(), IDLE)) { */
      /*   bytes_avail = cc1101_rx_fifo_bytes(); */
      /*   if(!bytes_avail) { */
      /*     cc1101_set_receive(); */
      /*   } */
      /* } */

      g_debug("TIMEOUT: current radio state: %s", cc1101_get_chip_state_str());
      g_debug("Bytes in Queue: %d", cc1101_rx_fifo_bytes());
      if (IS_STATE(cc1101_get_chip_state(), IDLE)) {
        bytes_avail = cc1101_rx_fifo_bytes();
        if(!bytes_avail) {
          cc1101_set_receive();
        }
      }
    } else {
      no_signal_cnt = 0;
      // in case of high background noise the PQT might be too small and noise is mistaking as valid data.
      // This should be the only case where the FIFO overflows so we just discard all received data.
      if (IS_STATE(cc1101_get_chip_state(), RXFIFO_OVERFLOW)) {
        g_debug("radio overflowed -> resetting");
        cc1101_command_strobe(header_command_sfrx);
        cc1101_set_receive();
      } else if (IS_STATE(cc1101_get_chip_state(), IDLE)) {
        bytes_avail = cc1101_rx_fifo_bytes();
        g_debug("bytes in queue: %d", bytes_avail);
        if(bytes_avail == 4 + 3) { // 4 bytes data + additional information (radio strength, ...)
          uint8_t read_buf[bytes_avail];
          uint32_t reference_timestamp_sec;

          cc1101_read_rx_fifo(read_buf, bytes_avail);
          memcpy(&reference_timestamp_sec, read_buf+1, sizeof(uint32_t));

          g_debug("Got reference seconds: %d\n", reference_timestamp_sec);

          reference_timestamp_pair = malloc(sizeof(timestamp_pair_t));
          reference_timestamp_pair->local_timestamp_ps = *local_timestamp_ps;
          reference_timestamp_pair->reference_timestamp_ps = reference_timestamp_sec * TIME_UNIT;

          g_async_queue_push(_timestamp_ref_queue, reference_timestamp_pair);
        }

        g_debug("state before reset: %s", cc1101_get_chip_state_str());
        cc1101_command_strobe(header_command_sfrx);
        cc1101_set_receive(); // we are ready to receive another package
        g_debug("state after reset: %s", cc1101_get_chip_state_str());
      }
    }
  }
}

int radio_init(GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue) {
  int handle; // handle system not implemented, use structure instead for one session?

  _timestamp_unref_queue  = timestamp_unref_queue;
  _timestamp_ref_queue  = timestamp_ref_queue;

  handle = cc1101_init("/dev/spidev0.0");

  if (handle < 0) return 0;

  while(IS_STATE(cc1101_get_chip_state(), SETTLING)) {
    g_debug("device is settling\n");
  }

  unsigned char PKTCTRL1_old;
  PKTCTRL1_old = cc1101_read_config(PKTCTRL1);
  cc1101_write_config(PKTCTRL1, PKTCTRL1_old | 0x08 | 0x80); // 0x08 -> auto CRC Flush, 0x40 -> PQT = 2*4 = 8
  cc1101_read_config(PKTCTRL1);

  // enable frequency synthesizer auto-calibration upon entering rx or tx state from idle every third time
  int MCSM0_old;
  MCSM0_old = cc1101_read_config(MCSM0);
  cc1101_write_config(MCSM0, MCSM0_old | 0x30);
  cc1101_read_config(MCSM0);

  cc1101_set_base_freq(1091553);// frequency increment for roughly 433MHz transmission

  // set relative carrier sense threshold to 6dB
  int AGCCTRL1_old;
  AGCCTRL1_old = cc1101_read_config(AGCCTRL1);
  cc1101_write_config(AGCCTRL1, AGCCTRL1_old | 0x20);
  cc1101_read_config(AGCCTRL1);

  // require both 16/16 sync word recognition aswell as positiver carrier sense threshold
  int MDMCFG2_old;
  MDMCFG2_old = cc1101_read_config(MDMCFG2);
  cc1101_write_config(MDMCFG2, MDMCFG2_old | 0x06);
  cc1101_read_config(MDMCFG2);

  // set GDO0 GPIO function to assert transmission of sync packet
  cc1101_write_config(IOCFG0, 0x07);

  // ensure device is in IDLE mode
  cc1101_command_strobe(header_command_sidle);
}

void radio_start_reception() {
  cc1101_command_strobe(header_command_sfrx);
  // calibrate the receiver once, after this we will calibrate every 4th RX <-> IDLE transition (see MCSM0)
  cc1101_command_strobe(header_command_scal);

  while(IS_STATE(cc1101_get_chip_state(), CALIBRATE)) {
    g_debug("device is calibrating\n");
  }

  cc1101_set_receive();

  g_thread_new("radio", radio_thread_func, NULL);
}
