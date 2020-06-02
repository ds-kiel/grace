#include "context.h"

#include "output.h"
#include "timestamp.h"

#include <libsigrok/libsigrok.h>

#include <stdio.h>
#include <stdbool.h>



static struct sr_context* sr_cntxt;
struct sr_session* sr_session;
struct sr_dev_inst* fx2ladw_dvc_instc;

static test_configuration_t* test_conf;

static char active_channel_mask = 0; // stream input is a byte with each channel represententing 1 bit

/* option a) handle every datafeed packet explicitly. This means if we only want to track falling or raising edges */
/* the logic has to be implemented here seperately. */
/* an alternative would be using sigrok triggers and checking */

void data_feed_callback(const struct sr_dev_inst *sdi,
                                            const struct sr_datafeed_packet *packet,
                                            void *cb_data) {
static bool initial_df_logic_packet = true;
static uint8_t last;
/* SR_API int sr_packet_copy(const struct sr_datafeed_packet *packet, */
		/* struct sr_datafeed_packet **copy); */
/* SR_API void sr_packet_free(struct sr_datafeed_packet *packet); */

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
        init_clock(test_conf, payload->length);
        last = dataArray[payload->length];
        initial_df_logic_packet = false;
      }
      /* printf("Got datafeed payload of length %lu. Unit size is %d byte\n", payload->length, payload->unitsize); */
      sample_count = 0;
      for(size_t i = 0; i < payload->length; i++) {
        if((dataArray[i]&active_channel_mask) != last) {
          for (size_t k = 0; k < test_conf->channel_count ; k++) {
            channel_configuration_t* c_conf = &test_conf->channels[k];
            char k_channel_mask = (1<<c_conf->channel);
            int8_t delta = (last&k_channel_mask) - (dataArray[i]&k_channel_mask);
            uint8_t state;
            if(delta) {
              printf("a channel changed state \n");
              if((c_conf->type & MATCH_FALLING) && delta > 0) { // falling signal
                state = 0;
              } else if (c_conf->type & MATCH_RISING){  // rising signal
                state = 1;
              }
              struct timespec ts;
              get_timestamp(&ts);
              timestamp_t data = {.channel = c_conf->channel, .state = state, .time = ts};
              write_sample(data);
            }
          }
          last = dataArray[i]&active_channel_mask;
        }
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
static int fx2_cleanup() {
  // uninitialize sigrok
  int ret;
  if ((ret = sr_exit(sr_cntxt)) != SR_OK) {
    printf("Error shutting down libsigkrok (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }
  close_output_file();

  return 0;
}


// current way of stopping the program is by terminating with SIGINT. This should probably be changed to some other method later
static void sigint_handler(int sig) {
  printf("received sigint!!\n");
  fx2_cleanup();
}

int fx2_init_instance(test_configuration_t* configuration)
{
  test_conf = configuration;

  // create channel mask
  for (size_t k = 0; k < test_conf->channel_count ; k++) {
    active_channel_mask+=1<<test_conf->channels[k].channel;
  }

  open_output_file(test_conf->logpath);
  //TODO Needs to handle the case that the session is not yet initialized;
  signal(SIGINT, sigint_handler);
  // initialize sigrok
  int ret;

  if ((ret = sr_init(&sr_cntxt)) != SR_OK) {
    printf("Error initializing libsigrok (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return 1;
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
    return 1;
  }

  if ((ret = sr_driver_init(sr_cntxt, fx2ladw_drv)) != SR_OK) {
    printf("Could not initialize driver fx2lafw!\n");
    return 1;
  }

  // try to initialize a device instance
  GSList* fx2ladw_dvcs;
  if((fx2ladw_dvcs = sr_driver_scan(fx2ladw_drv, NULL)) == NULL) {
    printf("Could not find any fx2ladw compatible devices\n");
    return 1;
  }
  fx2ladw_dvc_instc = fx2ladw_dvcs->data;

  if ((ret = sr_dev_open(fx2ladw_dvc_instc)) != SR_OK) {
    printf("Could not open device (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return 1;
  }

  /* --- Device configuration --- */
  // TODO currently unused -> remove later if not needed anymore
  GArray* fx2ladw_scn_opts;
  // TODO create Macro for this pattern
  if((fx2ladw_scn_opts = sr_driver_scan_options_list(fx2ladw_drv)) == NULL) {
    printf("Invalid arguments?\n");
    return 1;
  }

   GArray* fx2ladw_dvc_opts = sr_dev_options(fx2ladw_drv, fx2ladw_dvc_instc, NULL);
  // TODO handle multiple possible instances
  for(size_t k = 0; k < fx2ladw_dvc_opts->len; k++) {
    enum sr_configkey key = g_array_index(fx2ladw_dvc_opts, enum sr_configkey, k);
    printf("key: %d\n", key);

    if(key == SR_CONF_SAMPLERATE) {
      guint64 rate = configuration->samplerate;
      GVariant *data = g_variant_new_uint64 (rate);
      if((ret = sr_config_set(fx2ladw_dvc_instc, NULL, key, data))) {
        printf("Could not set samplerate (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
        return 1;
      }
      printf("successfully set sample rate to %lu\n", configuration->samplerate);
    }
    GVariant* values;
    switch (ret = sr_config_list(fx2ladw_drv, fx2ladw_dvc_instc, NULL, key, &values)) {
      case SR_ERR: {
        printf("Something did go wrong (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
        return 1;
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
  }
  g_array_free(fx2ladw_dvc_opts, TRUE);

  // setting up new sigrok session
  if ((ret = sr_session_new(sr_cntxt, &sr_session)) != SR_OK) {
    printf("Unable to create session (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return 1;
  }

  if ((ret = sr_session_dev_add(sr_session, fx2ladw_dvc_instc)) != SR_OK) {
    printf("Could not add device to instance (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return 1;
  }

  void* data;
  if ((ret = sr_session_datafeed_callback_add(sr_session, &data_feed_callback, data)) != SR_OK) {
    printf("Could not add device to instance (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return 1;
  }

  //cleanup
  g_array_free(fx2ladw_scn_opts, TRUE);
  g_slist_free(fx2ladw_dvcs);

  return 0;
}


// returns -1 if session could not be starten
int fx2_run_instance () {
  int ret;
  if ((ret = sr_session_start(sr_session)) != SR_OK) {
    printf("Could not start session  (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  if ((ret = sr_session_run(sr_session)) != SR_OK) {
    printf("Could not run session  (%s): %s.\n", sr_strerror_name(ret), sr_strerror(ret));
    return -1;
  }

  fx2_cleanup();

  return 0;
}
