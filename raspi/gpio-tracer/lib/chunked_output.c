#include <output_module.h>
#include <chunked_output.h>
#include <configuration.h>
#include <types.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <fcntl.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <inttypes.h>


/* #define G_LOG_DOMAIN "OUTPUT" */

/* typedef size_t jobid; */

typedef struct chunked_output {
  output_write_function write;

  gchar *data_path;
  GAsyncQueue *trace_queue;
  size_t chunk_cnt;
  /* jobid id; */

  struct trace *traces_mmap;
  char *curr_trace;
  size_t traces_file_size;
  size_t traces_written;
} chunked_output_t;


chunked_output_t* chunked_output_new() {
  return malloc(sizeof(chunked_output_t));
}


int chunked_output_deinit(chunked_output_t *output) {
  gchar *last_file;
  gint ret, fd;

  last_file =  g_strdup_printf("%s/traces-%zu", output->data_path, output->chunk_cnt-1);

  free(output->data_path);
  munmap(output->traces_mmap, output->traces_file_size);

  // truncate last file
  if ((fd = open(last_file, O_RDWR | O_CREAT, S_IRWXU)) < 0) {
    g_message("could not acquire next chunk");
    return -1;
  }

  if ((ret = ftruncate(fd, output->traces_written*TRACE_SIZE)) < 0) {
    g_message("Could not truncate file to required size of %d!", CHUNK_SIZE);
    return -1;
  }

  free(output);
  return 0;
}

// returns -1 on failure
int acquire_next_chunk(chunked_output_t *output) {
  gint ret, fd;
  gchar *target_file;

  // free current chunk if its existing
  if (output->traces_mmap != NULL) {
    munmap(output->traces_mmap, output->traces_file_size);
  }

  target_file = g_strdup_printf("%s/traces-%zu", output->data_path, output->chunk_cnt);

  if ((fd = open(target_file, O_RDWR | O_CREAT, S_IRWXU)) < 0) {
    g_message("could not acquire next chunk");
    return -1;
  }

  output->traces_file_size = CHUNK_SIZE * TRACE_SIZE;
  if ((ret = ftruncate(fd, output->traces_file_size)) < 0) {
    g_message("Could not truncate file to required size of %d!", CHUNK_SIZE);
    return -1;
  }

  if ((output->traces_mmap = mmap(NULL, output->traces_file_size, PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED,
                                  fd, 0)) == MAP_FAILED) {
    g_message("Could not mmap file into memory!");
    return -1;
  }

  g_message("Got memory mapped region at address %p", output->traces_mmap);

  if (close(fd)) {
    g_message("Could not close file");
    return -1;
  }

  output->curr_trace = (char*)output->traces_mmap;
  output->traces_written = 0;

  output->chunk_cnt++;

  return 0;
}

static void chunked_output_write(chunked_output_t *output, struct trace *sample) {
  g_message("writing state %d from channel %d with timestamp %" G_GINT64_FORMAT, sample->state, sample->channel, sample->timestamp_ns);
  /* memcpy(output->curr_trace, sample, TRACE_SIZE); */

  memcpy(output->curr_trace, &sample->channel, sizeof(sample->channel));
  output->curr_trace = output->curr_trace + sizeof(sample->channel);

  memcpy(output->curr_trace, &sample->state, sizeof(sample->state));
  output->curr_trace = output->curr_trace + sizeof(sample->state);

  memcpy(output->curr_trace, &sample->timestamp_ns, sizeof(sample->timestamp_ns));
  output->curr_trace = output->curr_trace + sizeof(sample->timestamp_ns);
  g_message("sizeof(sample->timestamp_ns) %lu", sizeof(sample->timestamp_ns));

  output->traces_written++;

  if (G_UNLIKELY(output->traces_written > CHUNK_SIZE)) {
    if(acquire_next_chunk(output) < 0) {
      g_message("Could not acquire next memory chunk at count %z!", output->chunk_cnt);
    }
  }
}


int chunked_output_init(chunked_output_t *output, const gchar* data_path) {
  int ret;
  size_t len;

  g_message("Initializing chunked output module!");

  output->chunk_cnt = 0;
  /* output->id = 0; // for now just 1 */

  len = strlen(data_path) + 1;
  output->data_path = malloc(sizeof(gchar)*len);
  memcpy(output->data_path, data_path, len);

  output->write = (output_write_function)chunked_output_write;

  if ((ret = acquire_next_chunk(output)) < 0) {
    g_message("Could not create initial chunk");
    return -1;
  }

  return 0;
}
