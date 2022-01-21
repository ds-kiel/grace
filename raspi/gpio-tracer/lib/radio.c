#include <radio.h>
#include <stdio.h>
#include <glib.h>
#include <types.h>
#include <configuration.h>

#include <cc1101.h>
#include <inttypes.h>
#include <time.h>
/* #include <fx2.h> */


static GThread *radio_thread = NULL;
static guint8 _running = 0;

/* static GAsyncQueue *_timestamp_ref_queue; // return queue for reference timestamps */
/* static GAsyncQueue *_timestamp_unref_queue; // process incoming gpio signals captured by the logic analyzer */

/* static gpointer radio_thread_func(gpointer data) { */
/*   static guint8 no_signal_cnt = 0; */

/*   _running = 1; */
/*   while(_running) { */
/*     int ret; */
/*     int bytes_avail; */
/*     guint64 *ref_time = NULL; */

/*     /\* ref_time = g_async_queue_timeout_pop(_timestamp_unref_queue, 2e6); *\/ */
/*     ref_time = g_async_queue_pop(_timestamp_unref_queue); */
/*     g_message("pop unreferenced timestamp from queue"); */
/*     g_usleep(1000); */

/*     if (_running==0) */
/*       break; */

/*     // Timeout -> check wether transceiver is in correct state */
/*     if(ref_time == NULL) { */
/*       g_message("TIMEOUT: current radio state: %s", cc1101_get_chip_state_str()); */
/*       g_message("Bytes in Queue: %d", cc1101_rx_fifo_bytes()); */
/*       if (IS_STATE(cc1101_get_chip_state(), IDLE)) { */
/*         bytes_avail = cc1101_rx_fifo_bytes(); */
/*         if(!bytes_avail) { */
/*           g_message("No bytes available but IDLE set back to receive!"); */
/*           cc1101_set_receive(); */
/*         } */
/*       } else if (IS_STATE(cc1101_get_chip_state(), RXFIFO_OVERFLOW)) { */
/*         cc1101_command_strobe(header_command_sfrx); */
/*         cc1101_set_receive(); */
/*       } */
/*     } else { */
/*       // in case of high background noise the PQT might be too small and noise is mistaking as valid data. */
/*       // This should be the only case where the FIFO overflows so we just discard all received data. */
/*       if (IS_STATE(cc1101_get_chip_state(), RXFIFO_OVERFLOW)) { */
/*         cc1101_command_strobe(header_command_sfrx); */
/*         for(int i = 0; i < 20; i++) { */
/*           g_message("radio overflowed -> resetting"); */
/*         } */
/*         *ref_time = 0; */
/*         g_async_queue_push(_timestamp_ref_queue, ref_time); */

/*       } else if (IS_STATE(cc1101_get_chip_state(), IDLE)) { */
/*         bytes_avail = cc1101_rx_fifo_bytes(); */
/*         g_message("bytes in queue: %d", bytes_avail); */
/*         if(bytes_avail == 7) { // 4 bytes data + additional information (radio strength, ...) */
/*           guint8 read_buf[bytes_avail]; */
/*           guint32 reference_timestamp_sec; */

/*           cc1101_read_rx_fifo(read_buf, bytes_avail); */
/*           memcpy(&reference_timestamp_sec, read_buf+1, sizeof(guint32)); */

/*           g_message("Got reference seconds: %d", reference_timestamp_sec); */

/*           /\* __prec_time_from_secs(reference_timestamp_sec, ref_time); *\/ */
/*           *ref_time = (guint64)reference_timestamp_sec; */

/*           g_async_queue_push(_timestamp_ref_queue, ref_time); */
/*         } else { // noise was incorrectly classified as valid packet */
/*           for(int i = 0; i < 20; i++) { */
/*             g_message("Wrong packet size!"); */
/*           } */
/*           cc1101_command_strobe(header_command_sfrx); */
/*           *ref_time = 0; */
/*           g_async_queue_push(_timestamp_ref_queue, ref_time); */
/*         } */

/*         g_message("state before reset: %s", cc1101_get_chip_state_str()); */

/*         cc1101_set_receive(); // we are ready to receive another package */
/*         g_message("state after reset: %s", cc1101_get_chip_state_str()); */
/*       } */
/*     } */
/*   } */
/* } */

guint64 radio_retrieve_timestamp() {
  int ret;
  int bytes_avail;
  guint64 ref_time;

  if (IS_STATE(cc1101_get_chip_state(), RXFIFO_OVERFLOW)) {
    cc1101_command_strobe(header_command_sfrx);
    for(int i = 0; i < 20; i++) {
      g_message("radio overflowed -> resetting");
    }
    ref_time = 0;
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
      ref_time = (guint64)reference_timestamp_sec;
    } else { // noise was incorrectly classified as valid packet
      for(int i = 0; i < 20; i++) {
        g_message("Wrong packet size!");
      }
      cc1101_command_strobe(header_command_sfrx);
      ref_time = 0;
    }

    g_message("state before reset: %s", cc1101_get_chip_state_str());

    cc1101_set_receive(); // we are ready to receive another package
    g_message("state after reset: %s", cc1101_get_chip_state_str());
  }

  return ref_time;
}

int radio_init(GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue) {
  int handle; // handle system not implemented, use structure instead for one session?
  cc1101_configuration_t cc1101_config;
  int ret;

  /* _timestamp_unref_queue  = timestamp_unref_queue; */
  /* _timestamp_ref_queue  = timestamp_ref_queue; */

  handle = cc1101_init("/dev/spidev0.0");

  if (handle < 0) return -1;

  while(IS_STATE(cc1101_get_chip_state(), SETTLING)) {
    g_message("device is settling");
  }



  cc1101_generate_reset_configuration(cc1101_config);
  if ((ret = cc1101_check_configuration_values(cc1101_config)) > 0) {
    g_message("Configuration after reset does not correspond with default configuration at addr: 0x%x", ret);
    // TODO remove see backlock
  }

  cc1101_set_base_freq(cc1101_config, 1091553);// frequency increment for roughly 433MHz transmission

  cc1101_high_sense_configuration(cc1101_config);
  cc1101_write_configuration(cc1101_config);

  if ((ret = cc1101_check_configuration_values(cc1101_config)) > 0) {
    g_message("Configuration after reset does not correspond with default configuration at addr: 0x%x", ret);
    // TODO remove see backlock
  }

  // ensure device is in IDLE mode
  cc1101_command_strobe(header_command_sidle);
  return 0;
}

int radio_deinit() {
  // TODO implement me
  _running = 0;
  /* g_thread_join(radio_thread); */

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

  /* radio_thread = g_thread_new("radio", radio_thread_func, NULL); */
}
