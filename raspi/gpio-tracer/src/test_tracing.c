#include <fx2.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>

#ifdef WITH_TIMESYNC
#include<radio.h>
#endif

#include <tracing.h>
#include <chunked_output.h>
#include <signal.h>

static volatile int keep_running = 1;

static gchar *trace_path;
static gchar *firmware_location;

static const GOptionEntry entries[] = {
  {"firmware", 'f', 0, G_OPTION_ARG_STRING, &firmware_location, "location for firmware (has to be in .bix format)", NULL},
  {"tracepath", 'p', 0, G_OPTION_ARG_STRING, &trace_path, "path for traces", "/tmp/"},
  { NULL }
};


void intHandler(int dummy) {
    keep_running = 0;
}

int print_bix = 0;

/* #define TRACE_PATH "/tmp/test-traces/" */

void packet_callback(uint8_t *packet_data, int length) {
  g_message("Got %d\n", length);
}

int main(int argc, char *argv[]) {
  GOptionContext *option_context;  
  GError *error;    
  tracing_instance_t *tracing_task;
  chunked_output_t *chunked_output;
  
  struct channel_configuration chan_conf = {
    .ch1 = SAMPLE_ALL,
    .ch2 = SAMPLE_ALL,
    .ch3 = SAMPLE_ALL,
    .ch4 = SAMPLE_ALL,
    .ch5 = SAMPLE_ALL,
    .ch6 = SAMPLE_ALL,
    .ch7 = SAMPLE_ALL,
    .ch8 = SAMPLE_RADIO,
  };


  option_context = g_option_context_new("control gpio tracer daemon");    
  g_option_context_add_main_entries(option_context, entries, NULL);
  if(!g_option_context_parse(option_context, &argc, &argv, &error)) {
    g_message("option parsing failed: %s\n", error->message);
    exit(1);
  }

  signal(SIGINT, intHandler);

  chunked_output = chunked_output_new();
  chunked_output_init(chunked_output, trace_path);

  tracing_task = tracing_init((output_module_t*) chunked_output, firmware_location);

  tracing_start(tracing_task, chan_conf);

  while (keep_running) { // TODO main loop
    tracing_handle_events(tracing_task);
  }

  print_free_running_clock_time(tracing_task);

  tracing_stop(tracing_task);

  tracing_deinit(tracing_task);

  sleep(4);

  chunked_output_deinit(chunked_output);

#ifdef WITH_TIMESYNC
  radio_deinit();
#endif

  g_option_context_free(option_context);  

  return 0;
}
