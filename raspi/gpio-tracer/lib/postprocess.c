#include <postprocess.h>
#include <configuration.h>
#include <types.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <glib/gprintf.h>
#include <inttypes.h>

static const gchar* _output_file_name;
static FILE *_fp;

static GAsyncQueue *_trace_queue;
static GAsyncQueue *_timestamp_ref_queue;

static void correct_timestamps_in_range(const timestamp_pair_t *ts_1, const timestamp_pair_t *ts_2) {
  trace_t *sample;
  double corr; // linear factor by which the timestamps have to be corrected

  corr = ((double)(ts_2->reference_timestamp_ns-ts_1->reference_timestamp_ns))/(ts_2->local_timestamp_ns - ts_1->local_timestamp_ns);
  /* g_printf("Fixing timestamps from %" PRIu64 " to %" PRIu64 " with corr: %f\n", ts_1->local_timestamp_ns, ts_2->local_timestamp_ns, corr); */
  while ((sample = g_async_queue_pop(_trace_queue))->timestamp_ns < ts_2->local_timestamp_ns) {
    if (sample->timestamp_ns > ts_1->local_timestamp_ns) {
      timestamp_t corrected_timestamp = ts_1->reference_timestamp_ns + (sample->timestamp_ns - ts_1->local_timestamp_ns)*corr;
      /* printf("New corrected timestamp: %" PRIu64 "\n"); */
      g_fprintf(_fp, "%" PRIu64 ",%d,%d\n", corrected_timestamp, sample->channel, sample->state);
      free(sample);
    }
  }
  g_async_queue_push_front (_trace_queue, sample); // last sample should be added again so it doesn't get dropped from processing
}

static gpointer postprocess_thread_func(gpointer data) {
  timestamp_pair_t *ts_1 = g_async_queue_pop(_timestamp_ref_queue);
  while(1) {
    timestamp_pair_t *ts_2 = g_async_queue_pop(_timestamp_ref_queue);

    correct_timestamps_in_range(ts_1, ts_2);

    free(ts_1);
    ts_1 = ts_2;
  }
}

  // free samples when writing into file

int postprocess_init(const gchar* logpath, GAsyncQueue* trace_queue, GAsyncQueue* timestamp_ref_queue) {
  // TODO pass reference to shared memory segment or threaded queue or whatever ...
  g_printf("Initializing postprocess thread!\n");

  _trace_queue = trace_queue;
  _timestamp_ref_queue = timestamp_ref_queue;

  if (open_output_file(logpath, TRUE) < 0) {
    g_printf("Unable to open output file %s\n", logpath);
    return -1;
  }

  g_thread_new("postprocess", postprocess_thread_func, NULL);
}

/* // should return -1 if failed */
/* static int flush_buffer_to_log() { */
/*     for(size_t k = 0; k < buf_index; k++) { */
/*       trace_t *sample = &write_buffer[k]; */
/*       g_fprintf(_fp, "%" PRIu64 ",%d,%d\n", sample->timestamp_ns, sample->channel, sample->state); */
/*     } */

/*     buf_index = 0; */
/*     return 1; */
/* } */

int open_output_file(const gchar* filename, gboolean overwrite) {
  _fp = fopen(filename, overwrite ? "w+" : "a+");
  g_fprintf(_fp, "#timestamp,channel,signal\n");
  if(_fp == NULL) return -1;

  /* write_buffer = (trace_t* ) malloc(sizeof(trace_t)*CONSTANT_SIZE_BUFFER_ENTRIES); */
  return 0;
}

int close_output_file() {
  /* flush_buffer_to_log(); */
  fclose(_fp);

  return 0;
}

int write_comment(char *comment) {
  /* flush_buffer_to_log(); // hack, write this data at some seperate section. For example header. */
  g_fprintf(_fp, "# %s \n", comment);
}

int write_sample(trace_t sample) {
  g_fprintf(_fp, "%" PRIu64 ",%d,%d\n", sample.timestamp_ns, sample.channel, sample.state);
  /* write_buffer[buf_index] = sample; */
  /* buf_index++; */
  /* if(buf_index >= CONSTANT_SIZE_BUFFER_ENTRIES) { */
  /*   flush_buffer_to_log(); */
  /* } */
  return 0;
}
