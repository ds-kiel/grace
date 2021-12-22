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
  static tracing_instance_t *tracing_task;
  static chunked_output_t *chunked_output;
  static GAsyncQueue *_timestamp_unref_queue;
  static GAsyncQueue *_timestamp_ref_queue;

  struct channel_configuration chan_conf = {
    .ch1 = SAMPLE_ALL,
    .ch2 = SAMPLE_ALL,
    .ch3 = SAMPLE_ALL,
    .ch8 = SAMPLE_RADIO,
  };

  if (argc < 2) {
    printf("missing arguments. Usage: test_fx2 <path_for_traces>\n");
    return 0;
  }

  signal(SIGINT, intHandler);

  _timestamp_unref_queue = g_async_queue_new();
  _timestamp_ref_queue   = g_async_queue_new();

  tracing_task = malloc(sizeof(tracing_instance_t));

  chunked_output = chunked_output_new();
  chunked_output_init(chunked_output, argv[1]);

#ifdef WITH_TIMESYNC
  g_message("Time synchronization enabled");

  radio_init(_timestamp_unref_queue, _timestamp_ref_queue);
#endif

  tracing_init(tracing_task, (output_module_t*) chunked_output, _timestamp_unref_queue, _timestamp_ref_queue);

  tracing_start(tracing_task, chan_conf);

#ifdef WITH_TIMESYNC
  g_message("Time synchronization enabled");

  radio_start_reception();
#endif

  while(keep_running) {
    ;;
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
