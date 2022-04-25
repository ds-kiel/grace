#include <stdint.h>
#include <string.h>
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
#include <assert.h>

#define TICKS_PER_SECOND ((guint64) 1 << 60)
#define TICKS_PER_NANOSECOND (((guint64) 1 << 60)/1000000000)

#define NEW_ESTIMATE_WEIGHT 0.90

int fw_send_start_cmd(struct fx2_device_manager *manager_instc);
int fw_send_stop_cmd(struct fx2_device_manager *manager_instc);
int tracing_fw_renumerate(struct fx2_device_manager *manager_instc);
int fw_status(struct fx2_device_manager *manager_instc, enum tracer_status_codes *status);
int fw_set_frequency(struct fx2_device_manager *manager, enum tracer_frequencies);
enum tracer_frequencies fw_get_frequency(struct fx2_device_manager *manager);
int firmware_wait(struct fx2_device_manager *manager_instc, enum tracer_status_codes code);

/* #define G_LOG_DOMAIN "TRACING" */

guint64 accumulator;
guint64 seconds;

void init_clock(tracing_instance_t *process) {
  uint32_t frequency = 48000000 / (process->current_freq+1);

  process->local_clock.nom_seq = frequency;
  process->local_clock.freq = 0;
  process->local_clock.state = WAIT;
  process->local_clock.free_seq = 0;
  process->local_clock.free_seq_cont = 0;
  process->local_clock.closed_seq = 0;

  process->local_clock.nom_frequency = TICKS_PER_SECOND / frequency;
  process->local_clock.adjusted_frequency = process->local_clock.nom_frequency;
  process->local_clock.res_error = 0;
  process->local_clock.offset_adj = 0;
  accumulator = 0;

  g_message("nominal frequency: %" G_GUINT64_FORMAT, process->local_clock.nom_frequency);
  g_message("adjusted frequency: %" G_GUINT64_FORMAT, process->local_clock.adjusted_frequency);
}

void update_clock_frequency(tracing_instance_t *process) {
  uint32_t frequency = 48000000 / (process->current_freq+1);

  process->local_clock.nom_seq = frequency;
  process->local_clock.nom_frequency = TICKS_PER_SECOND / frequency;
}

void tick(tracing_instance_t *process) {
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
    clk->free_seq_cont++; // TODO maybe remove again
  }
}

void print_free_running_clock_time(tracing_instance_t *process) {
  struct lclock *clk = &(process->local_clock);

  guint64 timestamp_ns = (clk->free_seq_cont * 125);

  g_message("current_time of free running clock: %" G_GUINT64_FORMAT, timestamp_ns);
}

// time reference signal handling
#ifdef WITH_TIMESYNC
void handle_time_ref_signal(tracing_instance_t *process) {
  struct lclock *clk = &(process->local_clock);
  guint64 ref_time;

  ref_time = radio_retrieve_timestamp();

  if (ref_time > 0) { // 0 -> invalid time ref
    guint64 time_since_last;

    switch (clk->state) {
    case WAIT: {
      clk->prev_time = ref_time;
      seconds = ref_time;

      clk->state = OFFSET;
      break;
    }
    case OFFSET: {
      time_since_last = ref_time - clk->prev_time;
      clk->adjusted_frequency = clk->nom_frequency*(((double)time_since_last*clk->nom_seq)/clk->free_seq);

      clk->state = FREQ;
      goto phase_error;
      break;
    }
    case FREQ: { // Main case. reached after two reference signals are received.
      frequency_error:
      time_since_last = ref_time - clk->prev_time;
      clk->adjusted_frequency = (1.0-NEW_ESTIMATE_WEIGHT)*clk->adjusted_frequency +  NEW_ESTIMATE_WEIGHT * clk->nom_frequency*(((double)time_since_last*clk->nom_seq)/clk->free_seq);

      /* other variant*/
      phase_error:
      // assumption clock is never off more than one second
      if (seconds == ref_time-1) {
        clk->offset = (TICKS_PER_SECOND - accumulator);
        clk->res_error = clk->offset;
        clk->offset *= -1;
      } else if (seconds == ref_time) {
        clk->offset = accumulator;
        clk->res_error = clk->offset;
      } else { // offset too large
        // implement me
        g_message("Offset too large this should not happen!");
        g_message("local seconds %" G_GUINT64_FORMAT, seconds);
        g_message("reference seconds %" G_GUINT64_FORMAT, ref_time);
      }

      g_message("free seq %llu", process->local_clock.free_seq);

      clk->offset_adj = clk->offset / (clk->nom_seq);
      clk->free_seq = 0;
      clk->prev_time = ref_time;

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
    /* g_debug("Discarded reference signal. Transceiver took to long to answer! \n"); */
  }
  /* free(ref_time); */
}
#endif

// GPIO signal handling
void handle_gpio_signal(tracing_instance_t *process, guint8 state, guint8 channel) {
  /* struct trace *trace = malloc(sizeof(struct trace)); */
#ifdef WITH_TIMESYNC
  if(process->local_clock.state==FREQ) {
#endif
    struct trace trace;
    trace.channel = channel;
    trace.state = state;
    /* trace.timestamp_ns = (1000000000 * seconds + 1e9 * (double)accumulator/TICKS_PER_SECOND); */
    trace.timestamp_ns = (1000000000 * seconds + (accumulator/TICKS_PER_NANOSECOND));

    g_message("Got change on channel %d", channel);
    // print trace using output module write function
    (*process->output->write)(process->output, &trace);
#ifdef WITH_TIMESYNC
  }
#endif
}

void data_feed_callback(uint8_t *packet_data, int length, void *user_data) {
  tracing_instance_t *process = (tracing_instance_t*)user_data;

  if(process->local_clock.free_seq_cont == 0) {
    process->prev_sample = packet_data[0];
  }

  for(int i = 0; i < length; i++) {
    guint8 curr = packet_data[i];

    if(curr != process->prev_sample) {
      guint8 changed = (process->prev_sample^curr) & process->active_channels_mask;
      guint8 state;

      if(changed) {
        for (size_t k = 0; k < sizeof(process->chan_conf.conf_arr); k++) {
          guint8 channel_mask = 1 << k;
          guint8 mode = process->chan_conf.conf_arr[k];

          if((changed & channel_mask) && mode != SAMPLE_NONE) {
#ifdef WITH_TIMESYNC
            if(G_UNLIKELY(mode & SAMPLE_RADIO) && (curr & channel_mask)) { // only match rising flank
              handle_time_ref_signal(process);
            }
#endif

            if((mode & SAMPLE_FALLING) && (process->prev_sample & channel_mask)) { // falling signal
              state = 0;
            } else if (mode & SAMPLE_RISING) {  // rising signal
              state = 1;
            }

            handle_gpio_signal(process, state, k+1);
          }
        }
      }
    }
    process->prev_sample = curr;
    tick(process);
  }
}

void tracer_datastream_finished_callback(void *user_data) {
  tracing_instance_t *process = (tracing_instance_t*) user_data;
  if(process->state == RUNNING) {
    g_message("data stream stopped unexpectedly. Try to reduce sampling rate");

    process->state = REDUCE_RATE;
  }
}

void firmware_reduce_frequency(tracing_instance_t *process) {
  fw_send_stop_cmd(&process->fx2_manager);

  firmware_wait(&process->fx2_manager, TRACER_GPIF_STOPPED);

  // the lowest possible frequency that we allow is 1MHz
  if (process->current_freq < FREQ_1000000) {
    process->current_freq++;

    fw_set_frequency(&process->fx2_manager, process->current_freq);

    init_clock(process);

    assert(fw_get_frequency(&process->fx2_manager) == process->current_freq);
  } else {
    g_message("Can't lower frequency any more. Stopping");
  }  
}

int fw_set_frequency(struct fx2_device_manager *manager, enum tracer_frequencies frequency) {
  int ret;
  unsigned char data[1] = { frequency };

  if ((ret = send_control_command(
                                  manager,
                                  LIBUSB_REQUEST_TYPE_VENDOR,
                                  VC_SET_DELAY, 0x00, 0, data, sizeof(data))) < 0) {
    g_error("could not set target sampling frequency:");
    return -1;
  }

  g_message("Successfully set tracer sampling delay to %d ", data[0]);

  return 0;
}

enum tracer_frequencies fw_get_frequency(struct fx2_device_manager *manager) {
  int ret;
  unsigned char read[1];

  if ((ret = send_control_command(
                                  manager,
                                  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_REQUEST_DIR_IN,
                                  VC_GET_DELAY, 0x00, 0, read, sizeof(read))) < 0) {
    g_error("could not get tracer sampling frequency:");
    return -1;
  }

  g_message("Got tracer frequency %d", read[0]);

  return read[0];
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

int fw_send_start_cmd(struct fx2_device_manager *manager_instc) {
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

int fw_send_stop_cmd(struct fx2_device_manager *manager_instc) { /*  */
  int ret;
  g_message("send stop sampling command to device");

  if ((ret = send_control_command(
                                  manager_instc,
                                  LIBUSB_REQUEST_TYPE_VENDOR,
                                  VC_STOP_SAMP, 0x00, 0, NULL, 0)) < 0) {
    g_error("could not stop tracing:");
    return -1;
  }

  return 0;
}

// hands of control of USB device request back to the fx2 default USB device
// this is done as (hopefully) working around a problem where sometimes
// the fx2 stops enumerating after conclusion of a test run (the testbed script toggles
// of the power of the integrated raspberry usb-hub).
int tracing_fw_renumerate(struct fx2_device_manager *manager_instc) {
  int ret;
  g_message("hand back USB request control to fx2 default USB device");

  if ((ret = send_control_command(
                                  manager_instc,
                                  LIBUSB_REQUEST_TYPE_VENDOR,
                                  VC_RENUM, 0x00, 0, NULL, 0)) < 0) {
    g_error("could not stop tracing:");
    return -1;
  }

  return 0;
}



// download is from the perspective of fx2lp, i.e., the firmware is downloaded into ram
int __download_tracer_firmware(tracing_instance_t *process, char* firmware_path) {
  struct fx2_device_manager *manager = &process->fx2_manager;
  // read firmware
  char *fname_bix = firmware_path;

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

  // write firmware to device
  fx2_cpu_set_reset(manager);

  g_message("uploading firmware");

  fx2_download_firmware(manager, bix, sizeof(bix), 1);

  /* fx2_upload_fw(manager, NULL, 0); */
  fx2_cpu_unset_reset(manager);
}

int fw_status(struct fx2_device_manager *manager_instc, enum tracer_status_codes *status) {
  int ret;
  unsigned char data[1];
  if ((ret = send_control_command(
                                  manager_instc,
                                  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_REQUEST_DIR_IN,
                                  VC_FWSTAT, 0x00, 0, data, 1)) < 0) {
    g_error("could not start tracing:");
    return -1;
  }

  g_message("firmware responded with status code: %d", data[0]);

  *status = (enum tracer_status_codes) data[0];

  return 0;
}

int firmware_wait(struct fx2_device_manager *manager_instc, enum tracer_status_codes code) {
  int retry;
  enum tracer_status_codes status;
  
  g_message("waiting for firmware");

  retry = 0;
  do {
    fw_status(manager_instc, &status);
    printf(".");
    sleep(1); 
  } while(status != code && retry++ < 4);
  printf("\n");

  if (retry >= 4) {
    g_message("firmware error");
    return -1;
  }

  return 0;
}


// TODO pass from higher layer
#define TMP_FIRMWARE_LOCATION "../firmware/fx2/build/fx2-tracer.bix"
int tracing_start(tracing_instance_t *process, struct channel_configuration channel_conf) {
  struct fx2_device_manager *manager = &process->fx2_manager;
  struct fx2_bulk_transfer_config *transfer_cnfg = &process->transfer_cnfg;
  enum tracer_status_codes tracer_state;

  process->chan_conf.conf = channel_conf;

  for (int k = 0; k < sizeof(process->chan_conf.conf_arr); k++) {
    if(process->chan_conf.conf_arr[k] != SAMPLE_NONE){
      process->active_channels_mask |= 1 << k;
    }
  }

  fx2_find_devices(manager);

  fx2_open_device(manager);

  /* unsigned char retry_count = 0; */
  /* while (retry_count++ < 10) { */
  /*   if(  fx2_get_status(manager) >= 0  ) { */
  /*     break; */
  /*   } */

  /*   fx2_close_device(manager); */

  /*   sleep(5); */

  /*   fx2_open_device(manager); */

  /*   sleep(1); */

  /*   fx2_reset_device(manager); */
  /* } */

  firmware_wait(&process->fx2_manager, TRACER_GPIF_STOPPED);

  sleep(1);

  fw_set_frequency(manager, process->current_freq);

  assert(fw_get_frequency(manager) == process->current_freq);

  fx2_create_bulk_transfer(manager, transfer_cnfg, 20, (1 << 18));
  fx2_set_bulk_transfer_packet_callback(transfer_cnfg, &data_feed_callback, (void *) process);
  fx2_set_bulk_transfer_finished_callback(transfer_cnfg, &tracer_datastream_finished_callback, (void *) process);
  fx2_submit_bulk_out_transfer(manager, transfer_cnfg);

  fw_send_start_cmd(manager);

  /* firmware_wait(&process->fx2_manager, TRACER_GPIF_RUNNING); */

  #ifdef WITH_TIMESYNC
  radio_start_reception();
  #endif

  process->state = RUNNING;

  return 0;
}

int tracing_stop(tracing_instance_t *process) {
  int ret;

  fw_send_stop_cmd(&process->fx2_manager);

  tracing_fw_renumerate(&process->fx2_manager);

  fx2_stop_bulk_out_transfer(&process->transfer_cnfg);

  fx2_close_device(&process->fx2_manager);

  fx2_deinit_manager(&process->fx2_manager);

  process->state = STOPPED;

  return 0;
}

tracing_instance_t* tracing_init(output_module_t *output) {
  int ret;
  tracing_instance_t *process;    
  struct fx2_device_manager *manager;

  process = malloc(sizeof(tracing_instance_t));
  memset(process, 0, sizeof(tracing_instance_t));

  manager = &process->fx2_manager;  

  process->current_freq = FREQ_24000000;

  /* // initialize local clock */
  init_clock(process);

  // enumerate candidate device
  fx2_init_manager(manager);
  
  fx2_find_devices(manager);
  fx2_open_device(manager);

  fx2_get_status(manager);

  sleep(1);

  __download_tracer_firmware(process, TMP_FIRMWARE_LOCATION);
  sleep(1);

  fx2_close_device(manager);
  sleep(2);
  
#ifdef WITH_TIMESYNC
  g_message("Time synchronization enabled");

  radio_init();
#endif
  
  process->output = output;

  return process;
}

int tracing_deinit(tracing_instance_t *process) {
  free(process);
  
#ifdef WITH_TIMESYNC
  radio_deinit();
#endif

  return 0;
}

void tracing_handle_events(tracing_instance_t *process) {
  fx2_handle_events(&process->fx2_manager);

  if(process->state == REDUCE_RATE) {
    firmware_reduce_frequency(process);
    
    fx2_submit_bulk_out_transfer(&process->fx2_manager, &process->transfer_cnfg);

#ifdef WITH_TIMESYNC
    radio_init();
#endif    

    fw_send_start_cmd(&process->fx2_manager);
    
#ifdef WITH_TIMESYNC
    radio_start_reception();
#endif        

    process->state = RUNNING;
  }
}
