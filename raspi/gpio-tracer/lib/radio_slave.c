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
    unref_timestamp = g_async_queue_timeout_pop(_timestamp_unref_queue, 250e3); // timeout 250 milliseconds

    // timeout check wether transceiver is in correct state
    if(unref_timestamp == NULL) {
      if (IS_STATE(cc1101_get_chip_state(), RXFIFO_OVERFLOW)) {
        cc1101_command_strobe(header_command_sfrx);
        cc1101_set_receive();
      }
    } else {

      // in case of high background noise the PQT might be too small and noise is mistaking as valid data.
      // This should be the only case where the FIFO overflows so we just discard all received data.
      if (IS_STATE(cc1101_get_chip_state(), RXFIFO_OVERFLOW)) {
        cc1101_command_strobe(header_command_sfrx);
        cc1101_set_receive();
      } else if (IS_STATE(cc1101_get_chip_state(), IDLE)) {
        bytes_avail = cc1101_rx_fifo_bytes();
        if(bytes_avail == 4 + 3) { // maybe change again later to 8 byte packets
          uint8_t read_buf[bytes_avail];
          uint32_t ref_timestamp_sec;

          cc1101_read_rx_fifo(read_buf, bytes_avail);
          // read ref timestamp from read_buf.
          ref_timestamp_pair = malloc(sizeof(timestamp_pair_t));
          ref_timestamp_pair->local_timestamp_ns = *unref_timestamp;
          memcpy(&ref_timestamp_sec, read_buf+1, sizeof(uint32_t));
          ref_timestamp_pair->reference_timestamp_ns = ref_timestamp_sec * 1e9;
          g_async_queue_push(_timestamp_ref_queue, ref_timestamp_pair);

          free(unref_timestamp); // data moved to wrapper structure so we can free it now
        }
        cc1101_command_strobe(header_command_sfrx);
        cc1101_set_receive(); // we are ready to receive another package
      }
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

void radio_slave_start_reception() {
  cc1101_command_strobe(header_command_sfrx);
  // calibrate the receiver once, after this we will calibrate every 4th RX <-> IDLE transition (see MCSM0)
  cc1101_command_strobe(header_command_scal);

  while(IS_STATE(cc1101_get_chip_state(), CALIBRATE)) {
    printf("device is calibrating\n");
  }

  cc1101_set_receive();

  g_thread_new("radio_slave", radio_slave_thread_func, NULL);
}
