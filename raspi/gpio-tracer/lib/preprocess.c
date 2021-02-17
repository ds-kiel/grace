#include <preprocess.h>

#include <postprocess.h>
#include <types.h>
#include <radio.h>
#include <configuration.h>

#include <libsigrok/libsigrok.h>
#include <glib/gprintf.h>

#include <stdio.h>
#include <stdbool.h>

static gboolean _running = FALSE;
static gboolean _node_is_slave = FALSE;

static struct sr_context* sr_cntxt;
struct sr_session* sr_session;
struct sr_dev_inst* fx2ladw_dvc_instc;

static char _active_channel_mask; // stream input is a byte with each channel represententing 1 bit
static guint8 _channel_count;

static struct channel_mode* _channels;
static guint32 _samplerate;
static double _nsec_per_sample; // TODO try long double performance

static GAsyncQueue* _trace_queue;
static GAsyncQueue* _timestamp_unref_queue;
static GAsyncQueue* _timestamp_ref_queue;

static inline timestamp_t timestamp_from_samples(guint64 sample_count) {
  return (timestamp_t) _nsec_per_sample * sample_count;
}


void data_feed_callback(const struct sr_dev_inst *sdi,
                                            const struct sr_datafeed_packet *packet,
                                            void *cb_data) {
  static gboolean initial_df_logic_packet = TRUE;
  static uint8_t last;
  /* static timestamp_t first_sync_timestamp; */
  static guint64 sample_count;


  switch (packet->type) {
    case SR_DF_HEADER: {
      struct sr_datafeed_header *payload = (struct sr_datafeed_header*) packet->payload;
      printf("got datafeed header: feed version %d, startime %lu\n", payload->feed_version, payload->starttime.tv_usec);
      initial_df_logic_packet = TRUE;
      sample_count = 0;
      _nsec_per_sample = (1.0/(double)_samplerate)*1e9;
      break;
    }
    case SR_DF_LOGIC: {
      struct sr_datafeed_logic *payload = (struct sr_datafeed_logic*) packet->payload;
      uint8_t* data = payload->data;

      if(initial_df_logic_packet) {
        last = data[0]&_active_channel_mask;
        initial_df_logic_packet = FALSE;
      }

      for(size_t i = 0; i < payload->length; i++) {
        /* printf("Got datafeed payload of length %" PRIu64 " Unit size is %" PRIu16 " byte\n", payload->length, payload->unitsize); */
        if((data[i]&_active_channel_mask) != last) {
          for (size_t k = 0; k < _channel_count ; k++) {
            uint8_t k_channel_mask = (1<<_channels[k].channel);
            int8_t delta = (last&k_channel_mask) - (data[i]&k_channel_mask);
            uint8_t state;
            if(delta) {
              if(_channels[k].channel == RECEIVER_CHANNEL) {
                if(delta < 1) { // Rising signal -> save timestamp

                  // In case this node is configured as slave we listen for incoming signals on the receiver channel and try to match them
                  // with a packet from the RX fifo queue through the radio-slave module by pushing our timestamped packet into the unref queue
                  if (_node_is_slave) {
                    g_printf("received signal on GPO0\n");
                    timestamp_t *timestamp_h = malloc(sizeof(timestamp_t));
                    *timestamp_h = timestamp_from_samples(sample_count);
                    g_async_queue_push(_timestamp_unref_queue, timestamp_h);
                  } else { // If our node acts as a master instead the radio-master emits the reference signals itself, so we will pop from the unref queue
                    timestamp_pair_t *timestamp_pair_ref = g_async_queue_try_pop(_timestamp_unref_queue);
                    // TODO this is blocking, there should never be a case where we record a receiver signal without a element in the queue. Nevertheless a maximum delay
                    // until the signal is unblocked a gain should be added.
                    if (timestamp_pair_ref != NULL) {
                      timestamp_pair_ref->local_timestamp_ns = timestamp_from_samples(sample_count);
                      g_async_queue_push(_timestamp_ref_queue, timestamp_pair_ref);
                    }
                  }
                }
              } else { // evaluate data from other channels
                /* g_printf("Channel %d changed state \n", _channels[k].channel); */
                if((_channels[k].mode & MATCH_FALLING) && delta > 0) { // falling signal
                  state = 0;
                } else if (_channels[k].mode & MATCH_RISING) {  // rising signal
                  state = 1;
                }

                trace_t *data = malloc(sizeof(trace_t));
                data->channel = _channels[k].channel;
                data->state = state;
                data->timestamp_ns = timestamp_from_samples(sample_count);

                g_async_queue_push(_trace_queue, data);
              }
            }
          }
        }
        last = data[i]&_active_channel_mask;
        sample_count++;
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

int preprocess_stop_instance() {
  int ret;

  // uninitialize sigrok
  free(_channels);
  if ((ret = sr_session_stop(sr_session)) != SR_OK) {
    printf("Error stopping sigrok session (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  return 0;
}

static int preprocess_kill_instance() {
  int ret;

  if(_running || sr_session == NULL) {
    g_printf("Unable to kill instance. Instance still _running or no session created.\n");
    return -1;
  }

  if ((ret = sr_session_destroy(sr_session)) != SR_OK) {
    g_printf("Error destroying sigrok session (%s): %s.\n", sr_strerror_name(ret),
             sr_strerror(ret));
    return -1;
  }

  if ((ret = sr_exit(sr_cntxt)) != SR_OK) {
    g_printf("Error shutting down libsigkrok (%s): %s.\n", sr_strerror_name(ret),
             sr_strerror(ret));
    return -1;
  }

  sr_session = NULL;
  sr_cntxt = NULL;

  g_printf("Successfully killed sigrok instance\n");

  return 0;
}


void session_stopped_callback(void* data) {
  g_printf("Session has stoppped!\n");
  close_output_file();
  _running = FALSE;
  preprocess_kill_instance();
}


void preprocess_set_type(const gchar* nodeType) {
  _node_is_slave = strcmp(nodeType, "master"); // TODO g_strcmp ???
}

// ownership of channel_modes is transfered to preprocess_init_sigrok(), so the GVariant should not be unreffed by the callee!
// returns -1 on failure. returns 1 if session already exists
static int preprocess_init_sigrok(guint32 samplerate)
{
  g_printf("Initializing sigrok instance\n");


  // initialize sigrok if none is already created
  if (sr_session != NULL) {
    return 1;
    g_printf("instance already initialized!\n");
  }

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
      guint32 rate = samplerate;
      GVariant *data = g_variant_new_uint64 (rate);
      if((ret = sr_config_set(fx2ladw_dvc_instc, NULL, key, data))) {
        printf("Could not set samplerate (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
        return -1;
      }
      _samplerate = samplerate;
      printf("successfully set sample rate to %" PRIu32 "\n", _samplerate);
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

gboolean preprocess_running() {
  return _running;
}

int preprocess_init(GVariant* channel_modes, GAsyncQueue *trace_queue, GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue) {
  int ret;

  if(_running) {
    g_printf("Instance already running!\n");
    return 1;
  }

  if (preprocess_init_sigrok(8000000) < 0) {
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
  _channels = malloc(length * sizeof(struct channel_mode));
  _channel_count = length;
  g_variant_get(channel_modes, "a(yy)", &iter);

  _active_channel_mask = 0;
  gint8 k = 0;
  while(g_variant_iter_loop(iter, "(yy)", &channel, &mode)) {
    g_printf("add channel %d with mode %d\n", channel, mode);
    _active_channel_mask += 1<<channel;
    _channels[k].channel = channel;
    _channels[k].mode = mode;
    k++;
  }

  g_variant_iter_free(iter);
  g_variant_unref(channel_modes);

  // set queue
  _trace_queue = trace_queue;
  _timestamp_unref_queue = timestamp_unref_queue;
  _timestamp_ref_queue = timestamp_ref_queue;

  if ((ret = sr_session_start(sr_session)) != SR_OK) {
    printf("Could not start session  (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  sr_session_stopped_callback_set(sr_session, &session_stopped_callback, NULL);

  _running = TRUE;

  return 0;
}
