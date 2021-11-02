#include <tracing.h>
#include <fx2.h>

#include <types.h>
#include <output_module.h>

#ifdef WITH_TIMESYNC
#include <radio.h>
#endif

#include <configuration.h>

#include <glib/gprintf.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#define TICKS_PER_SECOND ((guint64) 1 << 60)
#define TICKS_PER_NANOSECOND (((guint64) 1 << 60)/1000000000)

guint64 accumulator;
guint64 seconds;

void init_clock(tracing_instance_t* process, guint64 frequency) {
  process->local_clock.nom_freq = frequency;
  process->local_clock.freq = 0;
  process->local_clock.state = WAIT;
  process->local_clock.free_seq = 0;
  process->local_clock.closed_seq = 0;

  process->local_clock.nom_frequency = TICKS_PER_SECOND / frequency;
  process->local_clock.adjusted_frequency = process->local_clock.nom_frequency;
  process->local_clock.res_error = 0;
  process->local_clock.offset_adj = 0;
  accumulator = 0;

  g_message("nominal frequency: %" G_GUINT64_FORMAT, process->local_clock.nom_frequency);
  g_message("adjusted frequency: %" G_GUINT64_FORMAT, process->local_clock.adjusted_frequency);
}

void tick(tracing_instance_t *process) {
  static int seq_cnt = 0;
  struct lclock *clk = &(process->local_clock);

  if ((clk->state > WAIT)) {
    accumulator += clk->adjusted_frequency - clk->offset_adj;
    clk->res_error += clk->offset_adj * (clk->offset_adj < 0 ? 1 : -1);
    /* accumulator += clk->adjusted_frequency; */
    /* clk->res_error += clk->offset_adj * (clk->offset_adj < 0 ? 1 : -1);     */
    if(clk->res_error <= 0) {
      clk->offset_adj = 0;
    }

    if(accumulator > TICKS_PER_SECOND) {
      seconds++;
      accumulator -= TICKS_PER_SECOND;
    }

    // open-loop
    clk->free_seq++;
  }
  clk->free_seq++;  // TODO REMOVEMOVEMOVMEOMVOEMOVOMEOVE

  // ^
  // |
  // |
  // |
  // LKASJDLKASJDLKASJD
  // ALKJSDLKAJSDLKASJLDKJSA
  //LAKSJDKLAJSDKLASD
}

void print_free_running_clock_time(tracing_instance_t *process) {
  struct lclock *clk = &(process->local_clock);

  guint64 timestamp_ns = (clk->free_seq * 125);

  g_message("current_time of free running clock: %" G_GUINT64_FORMAT, timestamp_ns);  
}

// time reference signal handling
#ifdef WITH_TIMESYNC
static void handle_time_ref_signal(tracing_instance_t *process) {
  struct lclock *clk = &(process->local_clock);
  guint64 *ref_time = malloc(sizeof (guint64));

  g_debug("pushing new unreferenced timestamp to queue");
  g_async_queue_push(process->timestamp_unref_queue, ref_time);

  // read data from process
  g_debug("reading referenced timestamp from queue");
  ref_time = g_async_queue_pop(process->timestamp_ref_queue);

  if (ref_time > 0) { // 0 -> invalid time ref
    guint64 time_since_last;

    switch (clk->state) {
    case WAIT: {
      clk->prev_time = *ref_time;
      seconds = (*ref_time);

      clk->state = OFFSET;
      break;
    }
    case OFFSET: {
      time_since_last = *ref_time - clk->prev_time;
      clk->adjusted_frequency = clk->nom_frequency*(((double)time_since_last*clk->nom_freq)/clk->free_seq);

      clk->state = FREQ;
      goto phase_error;
      break;
    }
    case FREQ: { // Main case. reached after two reference signals are received.
      frequency_error:
      time_since_last = *ref_time - clk->prev_time;
      clk->adjusted_frequency = 0.3*clk->adjusted_frequency +  0.7 * clk->nom_frequency*(((double)time_since_last*clk->nom_freq)/clk->free_seq);

      /* other variant*/
      phase_error:
      // assumption clock is never off more than one second
      if (seconds == *ref_time-1) {
        clk->offset = (TICKS_PER_SECOND - accumulator);
        clk->res_error = clk->offset;
        clk->offset *= -1;
      } else if (seconds == *ref_time) {
        clk->offset = accumulator;
        clk->res_error = clk->offset;
      } else { // offset too large
        // implement me
        g_message("Offset too large this should not happen!");
        g_message("local seconds %" G_GUINT64_FORMAT, seconds);
        g_message("reference seconds %" G_GUINT64_FORMAT, *ref_time);
      }

      clk->offset_adj = clk->offset / (clk->nom_freq);
      clk->free_seq = 0;
      clk->prev_time = *ref_time;

      g_message("offset %" G_GINT64_FORMAT, clk->offset);
      g_message("offset_adj %" G_GINT64_FORMAT, clk->offset_adj);
      g_message("accumulator %" G_GUINT64_FORMAT, accumulator);
      g_message("adjusted frequency %" G_GUINT64_FORMAT, clk->adjusted_frequency);
      g_message("res_error %" G_GINT64_FORMAT, clk->res_error);

      break;
    }
default:
      break;
    }
  } else {
    g_debug("Discarded reference signal. Transceiver took to long to answer! \n");
  }
  free(ref_time);
}
#endif

// GPIO signal handling
static inline void handle_gpio_signal(tracing_instance_t *process, guint8 state, guint8 channel) {
  /* struct trace *trace = malloc(sizeof(struct trace)); */
  if(process->local_clock.state==FREQ) {
    struct trace trace;
    trace.channel = channel;
    trace.state = state;
    /* trace.timestamp_ns = (1000000000 * seconds + 1e9 * (double)accumulator/TICKS_PER_SECOND); */
    trace.timestamp_ns = (1000000000 * seconds + (accumulator/TICKS_PER_NANOSECOND));

    g_debug("writing trace to file");
    // print trace using output module write function
    (*process->output->write)(process->output, &trace);
  }
}

void data_feed_callback(uint8_t *packet_data, int length, void *user_data) {
  static gboolean initial_df_logic_packet = TRUE;
  static guint8 last;
  tracing_instance_t *process = (tracing_instance_t*)user_data;

  if(initial_df_logic_packet) {
    last = packet_data[0];
    initial_df_logic_packet = FALSE;
  }

  for(size_t i = 0; i < length; i++) {
    guint8 curr = packet_data[i];
    
    if(curr != last) {
      guint8 changed = (last^curr) & process->active_channels_mask;
      guint8 state;

      if(changed) {
        for (size_t k = 0; k < sizeof(process->chan_conf.conf_arr); k++) {
          guint8 channel_mask = 1 << (k-1);
          guint8 mode = process->chan_conf.conf_arr[k];

          if(changed & channel_mask && process->chan_conf.conf_arr[k] != SAMPLE_NONE) {
            if(G_UNLIKELY(process->chan_conf.conf_arr[k]==SAMPLE_RADIO) && (curr & channel_mask)) { // only match rising flank
              #ifdef WITH_TIMESYNC
              handle_time_ref_signal(process);
              #endif
            }

            if((mode & SAMPLE_FALLING) && (last & channel_mask)) { // falling signal
              state = 0;
            } else if (mode & SAMPLE_RISING) {  // rising signal
              state = 1;
            }

            handle_gpio_signal(process, state, k);
          }
        }
      }
    }
    last = curr;
    tick(process);
  }
}

int tracing_stop(tracing_instance_t *process) {
  int ret;

  process->state = STOPPED;

  return 0;
}

gboolean tracing_running(tracing_instance_t* process) {
  if (process == NULL)
    return 0;

  /* send_control_command(process->fx2_manager, */
  /*                      LIBUSB_RECIPIENT_DEVICE | LIBUSB_REQUEST_TYPE_STANDARD | */
  /*                      LIBUSB_REQUEST_HOST_TO_DEVICE, */
  /*                      LIBUSB_REQUEST_GET_STATUS, 0, 0, data, 2); */

  
  return process->state == RUNNING;

}

enum TRACER_VENDOR_COMMANDS {
  VC_START_SAMP = 0xB2,
  VC_STOP_SAMP,
  VC_SET_DELAY,
  VC_GET_DELAY,
};
  
static int __tracing_send_start_cmd(struct fx2_device_manager *manager_instc) {
  int ret;
  if ((ret = send_control_command(
                                  manager_instc,
                                  LIBUSB_REQUEST_TYPE_VENDOR,
                                  VC_START_SAMP, 0x00, 0, NULL, 0)) < 0) {
    g_error("could not start tracing:");
    return -1;
  }

    g_message("send start sampling command to device");

  return 0;
}

static int __tracing_send_stop_cmd(struct fx2_device_manager *manager_instc) {
  int ret;
  if ((ret = send_control_command(
                                  manager_instc,
                                  LIBUSB_REQUEST_TYPE_VENDOR,
                                  VC_STOP_SAMP, 0x00, 0, NULL, 0)) < 0) {
    g_error("could not start tracing:");
    return -1;
  }

    g_message("send start sampling command to device");

  return 0;
}


int tracing_start(tracing_instance_t *process, struct channel_configuration channel_conf) {
  struct fx2_device_manager *manager = &process->fx2_manager;

  process->chan_conf.conf = channel_conf;

  for (int i = 0; i < sizeof(process->chan_conf.conf_arr); i++) {
    process->active_channels_mask |= process->chan_conf.conf_arr[i];
  }
  
  fx2_find_devices(manager);

  fx2_open_device(manager);
  sleep(2);

  fx2_set_packet_callback(manager, &data_feed_callback, (void *) process);
  
  GThread *transfer_thread;
  transfer_thread = g_thread_new("bulk transfer thread", &fx2_transfer_loop_thread_func, (void *)manager);
  
  __tracing_send_start_cmd(manager);

  
  return 0;
}


#define TMP_FIRMWARE_LOCATION "../firmware/bulkloop.bix"

int tracing_init(tracing_instance_t *process, output_module_t *output, GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue) {
  int ret;

  if(process->state == RUNNING) {
    g_printf("Instance already running!\n");
    return 1;
  }

  /* // initialize local clock */
  init_clock(process, ANALYZER_FREQUENCY);

  // read firmware
  char *fname_bix = TMP_FIRMWARE_LOCATION;

  if (access(fname_bix, R_OK) != 0) {
    g_error("Could not read file %s", fname_bix);
    return -1;
  }

  FILE *f_bix;
  size_t f_size;
  f_bix = fopen(fname_bix, "rb");

  // determine size of file
  fseek(f_bix, 0, SEEK_END);
  f_size = ftell(f_bix);
  fseek(f_bix, 0, SEEK_SET);

  unsigned char bix[f_size];
  fread(bix, sizeof(bix), 1, f_bix);

  /* if(print_bix) */
    /* pretty_print_memory(bix, 0x0000, sizeof(bix)); */

  // enumerate candidate device
  struct fx2_device_manager *manager = &process->fx2_manager;

  fx2_init_manager(manager);
  fx2_find_devices(manager);
  fx2_open_device(manager);

  unsigned char data[2];

  // check status
  send_control_command(manager,
                       LIBUSB_RECIPIENT_DEVICE | LIBUSB_REQUEST_TYPE_STANDARD |
                           LIBUSB_REQUEST_HOST_TO_DEVICE,
                       LIBUSB_REQUEST_GET_STATUS, 0, 0, data, 2);

  g_message("Status: %x", data[0] << 4 | data[1]);

  // write firmware to device
  fx2_cpu_set_reset(manager);

  g_message("uploading firmware");

  fx2_download_firmware(manager, bix, sizeof(bix), 1);

  /* fx2_upload_fw(manager, NULL, 0); */
  fx2_cpu_unset_reset(manager);
  sleep(1);

  fx2_close_device(manager);
  sleep(2);

  /* // set queue */
  /* process->timestamp_unref_queue = timestamp_unref_queue; */
  /* process->timestamp_ref_queue = timestamp_ref_queue; */
  /* process->output = output; */



  /* /\* if ((ret = sr_session_start(process->sr_session)) != SR_OK) { *\/ */
  /* /\*   printf("Could not start session  (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret)); *\/ */
  /* /\*   return -1; *\/ */
  /* /\* } *\/ */

  /* process->state = RUNNING; */

  return 0;
}
