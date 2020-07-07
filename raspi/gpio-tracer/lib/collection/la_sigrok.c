#include "la_sigrok.h"

#include "../output.h"
#include "../types.h"

#include <libsigrok/libsigrok.h>
#include <glib/gprintf.h>

#include <stdio.h>
#include <stdbool.h>

static bool running = false;

static struct sr_context* sr_cntxt;
struct sr_session* sr_session;
struct sr_dev_inst* fx2ladw_dvc_instc;

static char active_channel_mask = 0; // stream input is a byte with each channel represententing 1 bit
gint8 channel_count;
struct channel_mode {gint8 channel; gint8 mode;};
static struct channel_mode* channels;
static uint64_t _samplerate = 0;
static double _nsec_per_frame;
static double _nsec_per_sample; // TODO double 64 bit type

long frame_count = 0; // used to determine time. TODO use another type
uint32_t sample_count = 0;

void timestamp_from_samples(long frame_count, uint32_t sample_count, struct timespec* ts_target) {
  long time_in_nsec = (frame_count-1) * _nsec_per_frame + _nsec_per_frame * sample_count;
  ts_target->tv_sec = time_in_nsec / 1e9;
  ts_target->tv_nsec = time_in_nsec % (long)1e9 - time_in_nsec;
}

void data_feed_callback(const struct sr_dev_inst *sdi,
                                            const struct sr_datafeed_packet *packet,
                                            void *cb_data) {
  static bool initial_df_logic_packet = true;
  static uint8_t last;
  switch (packet->type) {
    case SR_DF_HEADER: {
      struct sr_datafeed_header* payload = (struct sr_datafeed_header*) packet->payload;
      printf("got datafeed header: feed version %d, startime %lu\n", payload->feed_version, payload->starttime.tv_usec);
      break;
    }
    case SR_DF_LOGIC: {
      struct sr_datafeed_logic* payload = (struct sr_datafeed_logic*) packet->payload;
      uint8_t* dataArray = payload->data;

      // needed to determine block size ...,
      // it would be better to determine the block size some other way and embedded the information into some kind
      // of run_information structure. If the block size changes mid run we get a problem
      if(initial_df_logic_packet) {
        last = dataArray[payload->length];
        initial_df_logic_packet = false;
        _nsec_per_frame = _nsec_per_sample * payload->length;
      }

      frame_count++;

      printf("Got datafeed payload of length %" PRIu64 " Unit size is %" PRIu16 " byte\n", payload->length, payload->unitsize);
      sample_count = 0;
      for(size_t i = 0; i < payload->length; i++) {
        if((dataArray[i]&active_channel_mask) != last) {
          for (size_t k = 0; k < channel_count ; k++) {
            char k_channel_mask = (1<<channels[k].channel);
            int8_t delta = (last&k_channel_mask) - (dataArray[i]&k_channel_mask);
            uint8_t state;
            if(delta) {
              printf("a channel changed state \n");
              if((channels[k].mode & MATCH_FALLING) && delta > 0) { // falling signal
                state = 0;
              } else if (channels[k].mode & MATCH_RISING){  // rising signal
                state = 1;
              }
              struct timespec ts;
              timestamp_from_samples(frame_count, sample_count, &ts);
              timestamp_t data = {.channel = channels[k].channel, .state = state, .time = ts};
              write_sample(data);
            }
          }
          last = dataArray[i]&active_channel_mask;
        }
        sample_count++;
      }
      break;
    }
    default:
      printf("unknown payload type: %d\n", packet->type);

      // TODO undeterministically the stream ends with SR_DF_END
      //      a new context should be started in this case as a workaround for now
      // TODO determine wether this also happens with the original logic analyzers
      break;
  }
}

// return -1 if cleanup fails
int la_sigrok_stop_instance() {
  int ret;

  g_printf("Stopping sigrok instance\n");

  // uninitialize sigrok
  free(channels);
  if ((ret = sr_session_stop(sr_session)) != SR_OK) {
    printf("Error stopping sigrok session (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  close_output_file();

  return 0;
}

int la_sigrok_kill_instance() {
  int ret;

  if(running || sr_session == NULL) {
    g_printf("Unable to kill instance. Maybe the instance is still running?\n");
    return -1;
  }

  if ((ret = sr_session_destroy(sr_session)) != SR_OK) {
    printf("Error destroying sigrok session (%s): %s.\n", sr_strerror_name(ret),
           sr_strerror(ret));
    return -1;
  }

  if ((ret = sr_exit(sr_cntxt)) != SR_OK) {
    printf("Error shutting down libsigkrok (%s): %s.\n", sr_strerror_name(ret),
           sr_strerror(ret));
    return -1;
  }

  return 0;
}

// ownership of channel_modes is transfered to la_sigrok_init_instance(), so the GVariant should not be unreffed by the callee!
// returns -1 on failure. returns 1 if session already exists
int la_sigrok_init_instance(guint64 samplerate, const gchar* logpath, GVariant* channel_modes)
{
  g_printf("Initializing sigrok instance\n");

  if (sr_session != NULL) {
    return 1;
    g_printf("instance already initialized!\n");
  }

  // parse channel_modes to create active_channel_mask
  GVariantIter *iter;
  gint8 length;
  gint8 channel,mode;

  g_variant_get(channel_modes, "a(yy)", &iter);
  length = 0;
  while(g_variant_iter_loop(iter, "(yy)", &channel, &mode)) length++;
  g_variant_iter_free(iter);
  channels = malloc(length * sizeof(struct channel_mode));

  g_variant_get(channel_modes, "a(yy)", &iter);
  gint8 k = 0;
  while(g_variant_iter_loop(iter, "(yy)", &channel, &mode)) {
    g_printf("add channel %d with mode %d\n", channel, mode);
    active_channel_mask += 1<<channel;
    channels[k].channel = channel;
    channels[k].mode = mode;
    k++;
  }

  g_variant_iter_free(iter);
  g_variant_unref(channel_modes);

  if (open_output_file(logpath) < 0) {
    g_printf("Unable to open output file %s\n", logpath);
    return -1;
  }

  // initialize sigrok
  int ret;

  if ((ret = sr_init(&sr_cntxt)) != SR_OK) {
    printf("Error initializing libsigrok (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  printf("Host: %s\n", sr_buildinfo_host_get());
  //initialize fx2lafw

  struct sr_dev_driver** avbl_drvs;
  if ((avbl_drvs = sr_driver_list(sr_cntxt)) == NULL) {
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

  if ((ret = sr_driver_init(sr_cntxt, fx2ladw_drv)) != SR_OK) {
    printf("Could not initialize driver fx2lafw!\n");
    return -1;
  }

  // try to initialize a device instance
  GSList* fx2ladw_dvcs;
  if((fx2ladw_dvcs = sr_driver_scan(fx2ladw_drv, NULL)) == NULL) {
    printf("Could not find any fx2ladw compatible devices\n");
    return -1;
  }
  fx2ladw_dvc_instc = fx2ladw_dvcs->data;

  if ((ret = sr_dev_open(fx2ladw_dvc_instc)) != SR_OK) {
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

   GArray* fx2ladw_dvc_opts = sr_dev_options(fx2ladw_drv, fx2ladw_dvc_instc, NULL);
  // TODO handle multiple possible instances
  for(size_t k = 0; k < fx2ladw_dvc_opts->len; k++) {
    enum sr_configkey key = g_array_index(fx2ladw_dvc_opts, enum sr_configkey, k);
    printf("key: %d\n", key);

    GVariant* values;
    switch (ret = sr_config_list(fx2ladw_drv, fx2ladw_dvc_instc, NULL, key, &values)) {
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
      guint64 rate = samplerate;
      GVariant *data = g_variant_new_uint64 (rate);
      if((ret = sr_config_set(fx2ladw_dvc_instc, NULL, key, data))) {
        printf("Could not set samplerate (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
        return -1;
      }
      _samplerate = samplerate;
      _nsec_per_sample = (1/(double)_samplerate)*1e9;
      printf("successfully set sample rate to %" PRIu64 "\n", _samplerate)
    }
  }
  g_array_free(fx2ladw_dvc_opts, TRUE);

  // setting up new sigrok session
  if ((ret = sr_session_new(sr_cntxt, &sr_session)) != SR_OK) {
    printf("Unable to create session (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  if ((ret = sr_session_dev_add(sr_session, fx2ladw_dvc_instc)) != SR_OK) {
    printf("Could not add device to instance (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  void* data;
  if ((ret = sr_session_datafeed_callback_add(sr_session, &data_feed_callback, data)) != SR_OK) {
    printf("Could not add device to instance (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  //cleanup
  g_array_free(fx2ladw_scn_opts, TRUE);
  g_slist_free(fx2ladw_dvcs);

  return 0;
}

void session_stopped_callback(void* data) {
  printf("Session has stoppped!\n");
  running = false;
}

// returns -1 if session could not be starten
int la_sigrok_run_instance (void* args) {
  int ret;

  if ((ret = sr_session_start(sr_session)) != SR_OK) {
    printf("Could not start session  (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  sr_session_stopped_callback_set(sr_session, &session_stopped_callback, NULL);

  running = true;

  return 0;
}
