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

void intHandler(int dummy) {
    keep_running = 0;
}

int print_bix = 0;

/* #define TRACE_PATH "/tmp/test-traces/" */

void packet_callback(uint8_t *packet_data, int length) {
  g_message("Got %d\n", length);
}

int main(int argc, char *argv[]) {
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

  if (argc < 2) {
    printf("missing arguments. Usage: test_fx2 <path_for_traces>\n");
    return 0;
  }

  signal(SIGINT, intHandler);

  tracing_task = malloc(sizeof(tracing_instance_t));

  chunked_output = chunked_output_new();
  chunked_output_init(chunked_output, argv[1]);

  tracing_init(tracing_task, (output_module_t*) chunked_output);

  tracing_start(tracing_task, chan_conf);

  while (keep_running) { // TODO main loop
    tracing_handle_events(tracing_task);
  }

  print_free_running_clock_time(tracing_task);

  tracing_stop(tracing_task);

  sleep(4);

  chunked_output_deinit(chunked_output);

#ifdef WITH_TIMESYNC
  radio_deinit();
#endif

  return 0;
}
