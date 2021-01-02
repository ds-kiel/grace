#include <postprocess.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <glib/gprintf.h>
#include <inttypes.h>

static const gchar* output_file_name;
static FILE *fp;

static size_t buf_index = 0;
static GAsyncQueue* _trace_queue;
trace_t* write_buffer;

static gpointer postprocess_thread_func(gpointer data) {
  while(1) {
    trace_t *sample = g_async_queue_pop(_trace_queue);
    g_printf("Got sample from channel %d\n", sample->channel);
  }
}

int postprocess_init(const gchar* logpath, GAsyncQueue* trace_queue) {
  // TODO pass reference to shared memory segment or threaded queue or whatever ...
  g_printf("Initializing postprocess thread!\n");

  _trace_queue = trace_queue;

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
/*       g_fprintf(fp, "%" PRIu64 ",%d,%d\n", sample->timestamp_ns, sample->channel, sample->state); */
/*     } */

/*     buf_index = 0; */
/*     return 1; */
/* } */

int open_output_file(const gchar* filename, gboolean overwrite) {
  fp = fopen(filename, overwrite ? "w+" : "a+");
  g_fprintf(fp, "#timestamp,channel,signal\n");
  if(fp == NULL) return -1;

  /* write_buffer = (trace_t* ) malloc(sizeof(trace_t)*CONSTANT_SIZE_BUFFER_ENTRIES); */
  return 0;
}

int close_output_file() {
  /* flush_buffer_to_log(); */
  fclose(fp);
  free(write_buffer);

  return 0;
}

int write_comment(char *comment) {
  /* flush_buffer_to_log(); // hack, write this data at some seperate section. For example header. */
  g_fprintf(fp, "# %s \n", comment);
}

int write_sample(trace_t sample) {
  g_fprintf(fp, "%" PRIu64 ",%d,%d\n", sample.timestamp_ns, sample.channel, sample.state);
  /* write_buffer[buf_index] = sample; */
  /* buf_index++; */
  /* if(buf_index >= CONSTANT_SIZE_BUFFER_ENTRIES) { */
  /*   flush_buffer_to_log(); */
  /* } */
  return 0;
}
