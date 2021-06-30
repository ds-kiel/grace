#include <radio.h>
#include <stdio.h>
#include <glib.h>
#include <types.h>
#include <configuration.h>

#include <cc1101-spidev/cc1101.h>
#include <inttypes.h>
#include <time.h>

static GThread *radio_thread = NULL;
static guint8 _running = 0;

static GAsyncQueue *_timestamp_ref_queue; // return queue for reference timestamps
static GAsyncQueue *_timestamp_unref_queue; // process incoming gpio signals captured by the logic analyzer


/* static struct precision_time __prec_time_from_secs(guint64 seconds, struct precision_time *prec_time) { */
/*   prec_time->clk = seconds; */
/*   prec_time->acc = 0; */
/* } */

static gpointer radio_thread_func(gpointer data) {
  static uint8_t no_signal_cnt = 0;

  _running = 1;
  while(_running) {
    int ret;
    int bytes_avail;
    guint64 *ref_time = NULL; // add timestamp from tracing to identify pair

    ref_time = g_async_queue_timeout_pop(_timestamp_unref_queue, 2e6);
    /* local_timestamp_ps = g_async_queue_pop(_timestamp_unref_queue); */
    g_debug("pop unreferenced timestamp from queue");

    // Timeout -> check wether transceiver is in correct state
    if(ref_time == NULL) {
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
      } else if (IS_STATE(cc1101_get_chip_state(), RXFIFO_OVERFLOW)) {
        cc1101_command_strobe(header_command_sfrx);
        cc1101_set_receive();
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

          g_message("Got reference seconds: %d", reference_timestamp_sec);

          /* __prec_time_from_secs(reference_timestamp_sec, ref_time); */
          *ref_time = (guint64)reference_timestamp_sec;

          g_async_queue_push(_timestamp_ref_queue, ref_time);
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
    g_debug("device is settling");
  }

  unsigned char PKTCTRL1_old;
  PKTCTRL1_old = cc1101_read_config(PKTCTRL1);
  cc1101_write_config(PKTCTRL1, PKTCTRL1_old | 0x08 | 0x40); // 0x08 -> auto CRC Flush, 0x40 -> PQT = 2*4 = 8
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

int radio_deinit() {
  // TODO implement me
  _running = 0;
  g_thread_join(radio_thread);

  return 1;
}

void radio_start_reception() {
  cc1101_command_strobe(header_command_sfrx);
  // calibrate the receiver once, after this we will calibrate every 4th RX <-> IDLE transition (see MCSM0)
  cc1101_command_strobe(header_command_scal);

  while(IS_STATE(cc1101_get_chip_state(), CALIBRATE)) {
    g_debug("device is calibrating");
  }

  cc1101_set_receive();

  radio_thread = g_thread_new("radio", radio_thread_func, NULL);
}
