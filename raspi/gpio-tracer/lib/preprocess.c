#include <preprocess.h>

#include <postprocess.h>
#include <types.h>
#include <radio.h>
#include <configuration.h>


#include <glib/gprintf.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>


void init_clock(preprocess_instance_t* process, guint32 frequency) {
  /* process->local_clock = malloc(sizeof(lclock_t)); */
  process->local_clock.nom_freq = frequency;
  process->local_clock.nom_period = TIME_UNIT/frequency; // internal units -> 1 picosecond
  process->local_clock.period = process->local_clock.nom_period;
  process->local_clock.state = WAIT;
  process->local_clock.seq = 0;
  process->local_clock.prev_seq = 0;
}

void tick(preprocess_instance_t *process) {
  lclock_t *clk = &(process->local_clock);

  if (clk->state > WAIT) {
    if (clk->offset > clk->res_error) {
      clk->phase += clk->period + clk->res_error;
      clk->offset -= clk->res_error;
    }
    else {
      clk->phase += clk->period + clk->offset;
      clk->offset = 0;
    }
    clk->seq++;
  }
}

// time reference signal handling
static void handle_time_ref_signal(preprocess_instance_t *process) {
  timestamp_t *signal_cnt = malloc(sizeof(timestamp_t)); // TODO
  *signal_cnt = 1;
  timestamp_t *timestamp_local = NULL;
  timestamp_pair_t *reference_timestamp_pair = NULL;

  lclock_t *clk = &(process->local_clock);
  g_async_queue_push(process->timestamp_unref_queue, signal_cnt);

  // read data from process
  reference_timestamp_pair = g_async_queue_pop(process->timestamp_ref_queue); // if no answer after 500 milliseconds discard reference packet
  if (reference_timestamp_pair != NULL) {
    switch (clk->state) {
    case WAIT: {
      clk->state = OFFSET;
      clk->phase = reference_timestamp_pair->reference_timestamp_ps;
      clk->prev_ref_phase = clk->phase;
      break;
    }
    case OFFSET: {
      clk->state = FREQ;
      clk->freq = clk->nom_freq * (double)clk->phase/reference_timestamp_pair->reference_timestamp_ps;
      clk->period = TIME_UNIT/clk->freq;
      clk->prev_ref_phase = reference_timestamp_pair->reference_timestamp_ps;
      clk->phase = reference_timestamp_pair->reference_timestamp_ps;
      clk->prev_seq = clk->seq;
      printf("period %f\n", clk->period);
      break;
    }
    case FREQ: {
      timestamp_t offset;
      timestamp_t local_phase;
      timestamp_t ref_phase;

      ref_phase = reference_timestamp_pair->reference_timestamp_ps;
      local_phase =  clk->phase;
      offset = ref_phase - local_phase;

      // update elements
      clk->offset = offset;
      clk->res_error = offset < -100e11 ? -100e11 : offset > 100e11 ? 100e11 : offset; // per second the PLL can remove 100microseconds
      clk->res_error = clk->res_error/clk->nom_freq;


      /* clk->I += (double) offset; */
      // --- Working but not elegant ---
      /* clk->I = clk->I < -3e15 ? -3e15 : clk->I > 3e15 ? 3e15 : clk->I; */
      /* I = clk->I* (0.00000001); */
      /* P = (offset*0.002) * 0.000005; */
      /* clk->freq = clk->nom_freq + P + I; */

      clk->freq = clk->nom_freq * (double)((clk->seq * clk->nom_period) - (clk->prev_seq * clk->nom_period))/(ref_phase - clk->prev_ref_phase);
      clk->period = TIME_UNIT/clk->freq;
      /* clk->period -= I; */
      clk->prev_ref_phase = ref_phase;
      clk->prev_seq = clk->seq;

      printf("offset %" PRId64 "ns, period %" PRId64 "\n", (int64_t) offset/1000000, clk->period);
      break;
    }
default:
      break;
    }
  } else {
    g_printf("Discarded reference signal. Transceiver took to long to answer! \n");
  }
  free(signal_cnt);
}

// GPIO signal handling
static inline void handle_gpio_signal(preprocess_instance_t *process, uint8_t state, uint8_t channel) {
  trace_t *trace = malloc(sizeof(trace_t));
  trace->channel = channel;
  trace->state = state;
  trace->timestamp_ns = process->local_clock.phase;

  g_async_queue_push(process->trace_queue, trace);
}

void data_feed_callback(const struct sr_dev_inst *sdi,
                                            const struct sr_datafeed_packet *packet,
                                            void *cb_data) {
  static gboolean initial_df_logic_packet = TRUE;
  static uint8_t last;
  preprocess_instance_t *process = (preprocess_instance_t*) cb_data;
  /* static timestamp_t first_sync_timestamp; */

  switch (packet->type) {
    case SR_DF_HEADER: {
      struct sr_datafeed_header *payload = (struct sr_datafeed_header*) packet->payload;
      printf("got datafeed header: feed version %d, startime %lu\n", payload->feed_version, payload->starttime.tv_usec);
      initial_df_logic_packet = TRUE;

      break;
    }
    case SR_DF_LOGIC: {
      struct sr_datafeed_logic *payload = (struct sr_datafeed_logic*) packet->payload;
      uint8_t* data = payload->data;

      if(initial_df_logic_packet) {
        last = data[0] & process->active_channel_mask;
        initial_df_logic_packet = FALSE;
      }

      for(size_t i = 0; i < payload->length; i++) {
        /* printf("Got datafeed payload of length %" PRIu64 " Unit size is %" PRIu16 " byte\n", payload->length, payload->unitsize); */
        if((data[i] & process->active_channel_mask) != last) {
          for (size_t k = 0; k < process->channel_count ; k++) {
            uint8_t k_channel_mask = (1<<process->channels[k].channel);
            int8_t delta = ((last&k_channel_mask)>>process->channels[k].channel) - ((data[i]&k_channel_mask)>>process->channels[k].channel);
            uint8_t state;
            if(delta) {
              if(process->channels[k].channel == RECEIVER_CHANNEL) {
                if(delta < 0) { // Rising signal -> save timestamp
                  handle_time_ref_signal(process);
                }
              } else {
                // determine wether signal was falling or rising
                if((process->channels[k].mode & MATCH_FALLING) && delta > 0) { // falling signal
                  state = 0;
                } else if (process->channels[k].mode & MATCH_RISING) {  // rising signal
                  state = 1;
                }

                handle_gpio_signal(process, state, process->channels[k].channel);
              }
            }
          }
        }
        last = data[i] & process->active_channel_mask;
        tick(process);
      }
      break;
    }
    case SR_DF_END: {
      g_printf("datastream from device ended\n");
      break;
    }
    default:
      printf("unhandled payload type: %d\n", packet->type);
      break;
  }
}

int preprocess_stop_instance(preprocess_instance_t *process) {
  int ret;

  // uninitialize sigrok
  free(process->channels);
  if ((ret = sr_session_stop(process->sr_session)) != SR_OK) {
    printf("Error stopping sigrok session (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  return 0;
}

static int preprocess_kill_instance(preprocess_instance_t *process) {
  int ret;

  if(process->state==RUNNING || process->sr_session == NULL) {
    g_printf("Unable to kill instance. Instance still _running or no session created.\n");
    return -1;
  }

  if ((ret = sr_session_destroy(process->sr_session)) != SR_OK) {
    g_printf("Error destroying sigrok session (%s): %s.\n", sr_strerror_name(ret),
             sr_strerror(ret));
    return -1;
  }

  if ((ret = sr_exit(process->sr_cntxt)) != SR_OK) {
    g_printf("Error shutting down libsigkrok (%s): %s.\n", sr_strerror_name(ret),
             sr_strerror(ret));
    return -1;
  }

  process->sr_session = NULL;
  process->sr_cntxt = NULL;

  g_printf("Successfully killed sigrok instance\n");

  return 0;
}


void session_stopped_callback(void* data) {
  preprocess_instance_t* process= (preprocess_instance_t*) data;
  g_printf("Session has stoppped!\n");
  close_output_file();
  process->state = STOPPED;
  preprocess_kill_instance(process);
}


// ownership of channel_modes is transfered to preprocess_init_sigrok(), so the GVariant should not be unreffed by the callee!
// returns -1 on failure. returns 1 if session already exists
static int preprocess_init_sigrok(preprocess_instance_t *process, guint32 samplerate)
{
  int ret;

  g_printf("Initializing sigrok instance\n");


  if ((ret = sr_init(&(process->sr_cntxt))) != SR_OK) {
    printf("Error initializing libsigrok (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  printf("Host: %s\n", sr_buildinfo_host_get());
  //initialize fx2lafw

  struct sr_dev_driver** avbl_drvs;
  if ((avbl_drvs = sr_driver_list(process->sr_cntxt)) == NULL) {
    printf("No hardware drivers available\n");
  }

  struct sr_dev_driver* fx2ladw_drv = NULL;
  for(size_t k = 0; avbl_drvs[k] != NULL; k++) {
    if (!strcmp(avbl_drvs[k]->name, "fx2lafw")) {
          printf(" found driver %s\n", avbl_drvs[k]->name);
          fx2ladw_drv = avbl_drvs[k];
          break;
    }
  }

  if (fx2ladw_drv==NULL) {
    printf("Could not find driver fx2lafw!\n");
    return -1;
  }

  if ((ret = sr_driver_init(process->sr_cntxt, fx2ladw_drv)) != SR_OK) {
    printf("Could not initialize driver fx2lafw!\n");
    return -1;
  }

  // try to initialize a device instance
  GSList* fx2ladw_dvcs;
  if((fx2ladw_dvcs = sr_driver_scan(fx2ladw_drv, NULL)) == NULL) {
    printf("Could not find any fx2ladw compatible devices\n");
    return -1;
  }

  process->fx2ladw_dvc_instc = fx2ladw_dvcs->data;

  if ((ret = sr_dev_open(process->fx2ladw_dvc_instc)) != SR_OK) {
    printf("Could not open device (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  /* --- Device configuration --- */
  // TODO currently unused -> remove later if not needed anymore
  GArray* fx2ladw_scn_opts;
  // TODO create Macro for this pattern
  if((fx2ladw_scn_opts = sr_driver_scan_options_list(fx2ladw_drv)) == NULL) {
    printf("Invalid arguments?\n");
    return -1;
  }

   GArray* fx2ladw_dvc_opts = sr_dev_options(fx2ladw_drv, process->fx2ladw_dvc_instc, NULL);
  // TODO handle multiple possible instances
  for(size_t k = 0; k < fx2ladw_dvc_opts->len; k++) {
    enum sr_configkey key = g_array_index(fx2ladw_dvc_opts, enum sr_configkey, k);
    printf("key: %d\n", key);

    GVariant* values;
    switch (ret = sr_config_list(fx2ladw_drv, process->fx2ladw_dvc_instc, NULL, key, &values)) {
      case SR_ERR: {
        printf("Something did go wrong (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
        return -1;
        break;
      }
      case SR_ERR_ARG: {
        printf("Unable to retrieve config list for key %d. (%s): %s.\n", key, sr_strerror_name(ret), sr_strerror(ret));
        break;
      }
      case SR_OK: {
        printf("Possible values: \n");
        g_print(g_variant_print(values,TRUE));
        printf("\n");
      }
      default:
        break;
    }

    if(key == SR_CONF_SAMPLERATE) {
      guint32 rate = samplerate;
      GVariant *data = g_variant_new_uint64 (rate);
      if((ret = sr_config_set(process->fx2ladw_dvc_instc, NULL, key, data))) {
        printf("Could not set samplerate (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
        return -1;
      }

      printf("successfully set sample rate to %" PRIu32 "\n", samplerate);
    }
  }
  g_array_free(fx2ladw_dvc_opts, TRUE);

  // setting up new sigrok session
  if ((ret = sr_session_new(process->sr_cntxt, &(process->sr_session))) != SR_OK) {
    printf("Unable to create session (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  if ((ret = sr_session_dev_add(process->sr_session, process->fx2ladw_dvc_instc)) != SR_OK) {
    printf("Could not add device to instance (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  void* data = (void*) process;
  if ((ret = sr_session_datafeed_callback_add(process->sr_session, &data_feed_callback, data)) != SR_OK) {
    printf("Could not add datafeed callback to instance (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  if ((ret = sr_session_stopped_callback_set(process->sr_session, &session_stopped_callback, data)) != SR_OK) {
    printf("Could not add stopped callback to instance (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }


  //cleanup
  g_array_free(fx2ladw_scn_opts, TRUE);
  g_slist_free(fx2ladw_dvcs);

  return 0;
}

gboolean preprocess_running(preprocess_instance_t* process) {
  return process->state == RUNNING;
}

int preprocess_init(preprocess_instance_t *process, GVariant *channel_modes, GAsyncQueue *trace_queue, GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue) {
  int ret;

  if(process->state == RUNNING) {
    g_printf("Instance already running!\n");
    return 1;
  }

  if (preprocess_init_sigrok(process, ANALYZER_FREQUENCY) < 0) {
    g_printf("Could not initialize sigrok instance!\n");
    return -1;
  };

  // parse channel_modes to create _active_channel_mask
  GVariantIter *iter;
  gint8 length;
  gint8 channel,mode;

  g_variant_get(channel_modes, "a(yy)", &iter);
  length = 0;
  while(g_variant_iter_loop(iter, "(yy)", &channel, &mode)) length++;
  g_variant_iter_free(iter);
  process->channels = malloc(length * sizeof(struct channel_mode));
  process->channel_count = length;
  g_variant_get(channel_modes, "a(yy)", &iter);

  process->active_channel_mask = 0;
  gint8 k = 0;
  while(g_variant_iter_loop(iter, "(yy)", &channel, &mode)) {
    g_printf("add channel %d with mode %d\n", channel, mode);
    process->active_channel_mask += 1<<channel;
    process->channels[k].channel = channel;
    process->channels[k].mode = mode;
    k++;
  }

  g_variant_iter_free(iter);
  g_variant_unref(channel_modes);

  // set queue
  process->trace_queue = trace_queue;
  process->timestamp_unref_queue = timestamp_unref_queue;
  process->timestamp_ref_queue = timestamp_ref_queue;

  // initialize local clock
  init_clock(process, ANALYZER_FREQUENCY);

  if ((ret = sr_session_start(process->sr_session)) != SR_OK) {
    printf("Could not start session  (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  process->state = RUNNING;

  return 0;
}
