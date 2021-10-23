#include <tracing.h>

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

/* void data_feed_callback_efficient(const struct sr_dev_inst *sdi, */
/*                                             const struct sr_datafeed_packet *packet, */
/*                                             void *cb_data) { */
/*   static gboolean initial_df_logic_packet = TRUE; */
/*   static guint8 last; */
/*   tracing_instance_t *process = (tracing_instance_t*) cb_data; */
/*   /\* static guint64 first_sync_timestamp; *\/ */

/*   switch (packet->type) { */
/*     case SR_DF_HEADER: { */
/*       struct sr_datafeed_header *payload = (struct sr_datafeed_header*) packet->payload; */
/*       printf("got datafeed header: feed version %d, startime %lu\n", payload->feed_version, payload->starttime.tv_usec); */
/*       initial_df_logic_packet = TRUE; */

/*       break; */
/*     } */
/*     case SR_DF_LOGIC: { */
/*       struct sr_datafeed_logic *payload = (struct sr_datafeed_logic*) packet->payload; */
/*       guint8* data = payload->data; */

/*       if(initial_df_logic_packet) { */
/*         last = data[0]; */
/*         initial_df_logic_packet = FALSE; */
/*       } */

/*       for(size_t i = 0; i < payload->length; i++) { */
/*         guint8 curr = data[i]; */
/*         if(curr != last) { */
/*           guint8 changed = (last^curr) & process->active_channels_mask; */
/*           guint8 state; */

/*           if(changed) { */
/*             for (size_t k = 0; k < process->channel_count ; k++) { */
/*               guint8 channel = process->channels[k].channel; */
/*               guint8 channel_mask = 1 << channel-1; */
/*               guint8 mode = process->channels[k].mode; */

/*               if(changed & channel_mask) { */
/*                 if(G_UNLIKELY(channel==RECEIVER_CHANNEL) && (curr & channel_mask)) { // only match rising flank */
/*                   handle_time_ref_signal(process); */
/*                 } */

/*                 if((mode & MATCH_FALLING) && (last & channel_mask)) { // falling signal */
/*                   state = 0; */
/*                 } else if (mode & MATCH_RISING) {  // rising signal */
/*                   state = 1; */
/*                 } */

/*                 handle_gpio_signal(process, state, channel); */
/*               } */
/*             } */
/*           } */
/*         } */
/*         last = curr; */
/*         tick(process); */
/*       } */
/*       break; */
/*     } */
/*     case SR_DF_END: { */
/*       g_printf("datastream from device ended\n"); */
/*       break; */
/*     } */
/*     default: */
/*       printf("unhandled payload type: %d\n", packet->type); */
/*       break; */
/*   } */

/* } */

int tracing_stop_instance(tracing_instance_t *process) {
  int ret;

  // uninitialize sigrok
  free(process->channels);
  /* if ((ret = sr_session_stop(process->sr_session)) != SR_OK) { */
  /*   printf("Error stopping sigrok session (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret)); */
  /*   return -1; */
  /* } */

  process->state = STOPPED;

  return 0;
}

static int tracing_kill_instance(tracing_instance_t *process) {
  /* int ret; */

  /* if(process->state==RUNNING || process->sr_session == NULL) { */
  /*   g_printf("Unable to kill instance. Instance still _running or no session created.\n"); */
  /*   return -1; */
  /* } */


  /* // close device */
  /* if ((ret = sr_dev_close(process->fx2ladw_dvc_instc))) { */
  /*   g_printf("Error closing device (%s): %s.\n", sr_strerror_name(ret), */
  /*            sr_strerror(ret)); */
  /*   return -1; */
  /* } */

  /* if ((ret = sr_session_destroy(process->sr_session)) != SR_OK) { */
  /*   g_printf("Error destroying sigrok session (%s): %s.\n", sr_strerror_name(ret), */
  /*            sr_strerror(ret)); */
  /*   return -1; */
  /* } */

  /* if ((ret = sr_exit(process->sr_cntxt)) != SR_OK) { */
  /*   g_printf("Error shutting down libsigkrok (%s): %s.\n", sr_strerror_name(ret), */
  /*            sr_strerror(ret)); */
  /*   return -1; */
  /* } */

  /* process->sr_session = NULL; */
  /* process->sr_cntxt = NULL; */

  /* g_printf("Successfully killed sigrok instance\n"); */

  return 0;
}


/* void session_stopped_callback(void* data) { */
  /* tracing_instance_t* process= (tracing_instance_t*) data; */
  /* g_printf("Session has stoppped!\n"); */
  /* process->state = STOPPED; */
  /* tracing_kill_instance(process); */
/* } */


// ownership of channel_modes is transfered to tracing_init_sigrok(), so the GVariant should not be unreffed by the callee!
// returns -1 on failure. returns 1 if session already exists
static int tracing_init_sigrok(tracing_instance_t *process, guint32 samplerate)
{
  /* int ret; */

  /* g_printf("Initializing sigrok instance\n"); */


  /* if ((ret = sr_init(&(process->sr_cntxt))) != SR_OK) { */
  /*   printf("Error initializing libsigrok (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret)); */
  /*   return -1; */
  /* } */

  /* printf("Host: %s\n", sr_buildinfo_host_get()); */
  /* //initialize fx2lafw */

  /* struct sr_dev_driver** avbl_drvs; */
  /* if ((avbl_drvs = sr_driver_list(process->sr_cntxt)) == NULL) { */
  /*   printf("No hardware drivers available\n"); */
  /* } */

  /* struct sr_dev_driver* fx2ladw_drv = NULL; */
  /* for(size_t k = 0; avbl_drvs[k] != NULL; k++) { */
  /*   if (!strcmp(avbl_drvs[k]->name, "fx2lafw")) { */
  /*         printf(" found driver %s\n", avbl_drvs[k]->name); */
  /*         fx2ladw_drv = avbl_drvs[k]; */
  /*         break; */
  /*   } */
  /* } */

  /* if (fx2ladw_drv==NULL) { */
  /*   printf("Could not find driver fx2lafw!\n"); */
  /*   return -1; */
  /* } */

  /* if ((ret = sr_driver_init(process->sr_cntxt, fx2ladw_drv)) != SR_OK) { */
  /*   printf("Could not initialize driver fx2lafw!\n"); */
  /*   return -1; */
  /* } */

  /* // try to initialize a device instance */
  /* GSList* fx2ladw_dvcs; */
  /* if((fx2ladw_dvcs = sr_driver_scan(fx2ladw_drv, NULL)) == NULL) { */
  /*   printf("Could not find any fx2ladw compatible devices\n"); */
  /*   return -1; */
  /* } */

  /* process->fx2ladw_dvc_instc = fx2ladw_dvcs->data; */

  /* if ((ret = sr_dev_open(process->fx2ladw_dvc_instc)) != SR_OK) { */
  /*   printf("Could not open device (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret)); */
  /*   return -1; */
  /* } */

  /* /\* --- Device configuration --- *\/ */
  /* // TODO currently unused -> remove later if not needed anymore */
  /* GArray* fx2ladw_scn_opts; */
  /* // TODO create Macro for this pattern */
  /* if((fx2ladw_scn_opts = sr_driver_scan_options_list(fx2ladw_drv)) == NULL) { */
  /*   printf("Invalid arguments?\n"); */
  /*   return -1; */
  /* } */

  /*  GArray* fx2ladw_dvc_opts = sr_dev_options(fx2ladw_drv, process->fx2ladw_dvc_instc, NULL); */
  /* // TODO handle multiple possible instances */
  /* for(size_t k = 0; k < fx2ladw_dvc_opts->len; k++) { */
  /*   enum sr_configkey key = g_array_index(fx2ladw_dvc_opts, enum sr_configkey, k); */
  /*   printf("key: %d\n", key); */

  /*   GVariant* values; */
  /*   switch (ret = sr_config_list(fx2ladw_drv, process->fx2ladw_dvc_instc, NULL, key, &values)) { */
  /*     case SR_ERR: { */
  /*       printf("Something did go wrong (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret)); */
  /*       return -1; */
  /*       break; */
  /*     } */
  /*     case SR_ERR_ARG: { */
  /*       printf("Unable to retrieve config list for key %d. (%s): %s.\n", key, sr_strerror_name(ret), sr_strerror(ret)); */
  /*       break; */
  /*     } */
  /*     case SR_OK: { */
  /*       printf("Possible values: \n"); */
  /*       g_print(g_variant_print(values,TRUE)); */
  /*       printf("\n"); */
  /*     } */
  /*     default: */
  /*       break; */
  /*   } */

  /*   if(key == SR_CONF_SAMPLERATE) { */
  /*     guint32 rate = samplerate; */
  /*     GVariant *data = g_variant_new_uint64 (rate); */
  /*     if((ret = sr_config_set(process->fx2ladw_dvc_instc, NULL, key, data))) { */
  /*       printf("Could not set samplerate (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret)); */
  /*       return -1; */
  /*     } */

  /*     printf("successfully set sample rate to %" PRIu32 "\n", samplerate); */
  /*   } */
  /* } */
  /* g_array_free(fx2ladw_dvc_opts, TRUE); */

  // setting up new sigrok session
  /* if ((ret = sr_session_new(process->sr_cntxt, &(process->sr_session))) != SR_OK) { */
  /*   printf("Unable to create session (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret)); */
  /*   return -1; */
  /* } */

  /* if ((ret = sr_session_dev_add(process->sr_session, process->fx2ladw_dvc_instc)) != SR_OK) { */
  /*   printf("Could not add device to instance (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret)); */
  /*   return -1; */
  /* } */

  /* void* data = (void*) process; */
  /* if ((ret = sr_session_datafeed_callback_add(process->sr_session, &data_feed_callback_efficient, data)) != SR_OK) { */
  /*   printf("Could not add datafeed callback to instance (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret)); */
  /*   return -1; */
  /* } */

  /* if ((ret = sr_session_stopped_callback_set(process->sr_session, &session_stopped_callback, data)) != SR_OK) { */
  /*   printf("Could not add stopped callback to instance (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret)); */
  /*   return -1; */
  /* } */


  //cleanup
  /* g_array_free(fx2ladw_scn_opts, TRUE); */
  /* g_slist_free(fx2ladw_dvcs); */

  return 0;
}

gboolean tracing_running(tracing_instance_t* process) {
  if (process == NULL)
    return 0;

  return process->state == RUNNING;
}

int tracing_init(tracing_instance_t *process, output_module_t *output, GVariant *channel_modes, GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue) {
  int ret;

  /* if(process->state == RUNNING) { */
  /*   g_printf("Instance already running!\n"); */
  /*   return 1; */
  /* } */

  /* if (tracing_init_sigrok(process, ANALYZER_FREQUENCY) < 0) { */
  /*   g_printf("Could not initialize sigrok instance!\n"); */
  /*   return -1; */
  /* }; */

  /* // parse channel_modes to create _active_channel_mask */
  /* GVariantIter *iter; */
  /* gint8 length; */
  /* gint8 channel,mode; */

  /* g_variant_get(channel_modes, "a(yy)", &iter); */
  /* length = 0; */
  /* while(g_variant_iter_loop(iter, "(yy)", &channel, &mode)) length++; */
  /* g_variant_iter_free(iter); */
  /* process->channels = malloc(length * sizeof(struct channel_mode)); */
  /* process->channel_count = length; */
  /* g_variant_get(channel_modes, "a(yy)", &iter); */

  /* process->active_channels_mask = 0; */
  /* gint8 k = 0; */
  /* while(g_variant_iter_loop(iter, "(yy)", &channel, &mode)) { */
  /*   g_printf("add channel %d with mode %d\n", channel, mode); */
  /*   process->active_channels_mask += 1<<channel-1; */
  /*   process->channels[k].channel = channel; */
  /*   process->channels[k].mode = mode; */
  /*   k++; */
  /* } */

  /* g_variant_iter_free(iter); */
  /* g_variant_unref(channel_modes); */

  /* // set queue */
  /* process->timestamp_unref_queue = timestamp_unref_queue; */
  /* process->timestamp_ref_queue = timestamp_ref_queue; */
  /* process->output = output; */

  /* // initialize local clock */
  /* init_clock(process, ANALYZER_FREQUENCY); */

  /* /\* if ((ret = sr_session_start(process->sr_session)) != SR_OK) { *\/ */
  /* /\*   printf("Could not start session  (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret)); *\/ */
  /* /\*   return -1; *\/ */
  /* /\* } *\/ */

  /* process->state = RUNNING; */

  return 0;
}
