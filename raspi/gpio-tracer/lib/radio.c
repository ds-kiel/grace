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

/* #define G_LOG_DOMAIN "RADIO" */

static gpointer radio_thread_func(gpointer data) {
  static guint8 no_signal_cnt = 0;

  _running = 1;
  while(_running) {
    int ret;
    int bytes_avail;
    guint64 *ref_time = NULL;

    /* ref_time = g_async_queue_timeout_pop(_timestamp_unref_queue, 2e6); */
    ref_time = g_async_queue_pop(_timestamp_unref_queue);
    g_message("pop unreferenced timestamp from queue");
    g_usleep(1000);

    // Timeout -> check wether transceiver is in correct state
    if(ref_time == NULL) {
      g_message("TIMEOUT: current radio state: %s", cc1101_get_chip_state_str());
      g_message("Bytes in Queue: %d", cc1101_rx_fifo_bytes());
      if (IS_STATE(cc1101_get_chip_state(), IDLE)) {
        bytes_avail = cc1101_rx_fifo_bytes();
        if(!bytes_avail) {
          g_message("No bytes available but IDLE set back to receive!");
          cc1101_set_receive();
        }
      } else if (IS_STATE(cc1101_get_chip_state(), RXFIFO_OVERFLOW)) {
        cc1101_command_strobe(header_command_sfrx);
        cc1101_set_receive();
      }
    } else {
      // in case of high background noise the PQT might be too small and noise is mistaking as valid data.
      // This should be the only case where the FIFO overflows so we just discard all received data.
      if (IS_STATE(cc1101_get_chip_state(), RXFIFO_OVERFLOW)) {
        cc1101_command_strobe(header_command_sfrx);
        for(int i = 0; i < 20; i++) {
          g_message("radio overflowed -> resetting");
        }
        *ref_time = 0;
        g_async_queue_push(_timestamp_ref_queue, ref_time);

      } else if (IS_STATE(cc1101_get_chip_state(), IDLE)) {
        bytes_avail = cc1101_rx_fifo_bytes();
        g_message("bytes in queue: %d", bytes_avail);
        if(bytes_avail == 7) { // 4 bytes data + additional information (radio strength, ...)
          guint8 read_buf[bytes_avail];
          guint32 reference_timestamp_sec;

          cc1101_read_rx_fifo(read_buf, bytes_avail);
          memcpy(&reference_timestamp_sec, read_buf+1, sizeof(guint32));

          g_message("Got reference seconds: %d", reference_timestamp_sec);

          /* __prec_time_from_secs(reference_timestamp_sec, ref_time); */
          *ref_time = (guint64)reference_timestamp_sec;

          g_async_queue_push(_timestamp_ref_queue, ref_time);
        } else { // noise was incorrectly classified as valid packet
          for(int i = 0; i < 20; i++) {
            g_message("Wrong packet size!");
          }
          cc1101_command_strobe(header_command_sfrx);
          *ref_time = 0;
          g_async_queue_push(_timestamp_ref_queue, ref_time);
        }

        g_message("state before reset: %s", cc1101_get_chip_state_str());

        cc1101_set_receive(); // we are ready to receive another package
        g_message("state after reset: %s", cc1101_get_chip_state_str());
      }
    }
  }
}

int radio_init(GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue) {
  int handle; // handle system not implemented, use structure instead for one session?

  _timestamp_unref_queue  = timestamp_unref_queue;
  _timestamp_ref_queue  = timestamp_ref_queue;

  handle = cc1101_init("/dev/spidev0.0");

  if (handle < 0) return -1;

  while(IS_STATE(cc1101_get_chip_state(), SETTLING)) {
    g_message("device is settling");
  }

  cc1101_set_base_freq(1091553);// frequency increment for roughly 433MHz transmission

  unsigned char PKTCTRL1_old,PKTCTRL1_new;
  PKTCTRL1_old = cc1101_read_config(PKTCTRL1);
  PKTCTRL1_new =
    2 << 5 | // PQT (preamble quality estimator threshold)
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

  // TODO Change modulation format

  // ensure device is in IDLE mode
  cc1101_command_strobe(header_command_sidle);
  return 0;
}

int radio_deinit() {
  // TODO implement me
  _running = 0;
  g_thread_join(radio_thread);

  return 0;
}

void radio_start_reception() {
  cc1101_command_strobe(header_command_sfrx);
  // calibrate the receiver once, after this we will calibrate every 4th RX <-> IDLE transition (see MCSM0)
  cc1101_command_strobe(header_command_scal);

  while(IS_STATE(cc1101_get_chip_state(), CALIBRATE)) {
    g_message("device is calibrating");
  }

  cc1101_set_receive();

  radio_thread = g_thread_new("radio", radio_thread_func, NULL);
}
