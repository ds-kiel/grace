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

typedef size_t jobid;

typedef struct chunked_output {
  output_write_function write;

  gchar *data_path;
  GAsyncQueue *trace_queue;
  size_t chunk_cnt;
  jobid id;

  trace_t *traces_mmap;
  trace_t *curr_trace;
  size_t traces_file_size;
  size_t traces_written;
} chunked_output_t;


chunked_output_t* chunked_output_new() {
  return malloc(sizeof(chunked_output_t));
}


int chunked_output_deinit(chunked_output_t *output) {
  free(output->data_path);
  munmap(output->traces_mmap, output->traces_file_size);

  free(output);
  return 0;
}

// returns -1 on failure
int acquire_next_chunk(chunked_output_t *output) {
  gint ret;
  GError *err = NULL;
  gint fd;
  gchar *target_file;

  // free current chunk if its existing
  if (output->traces_mmap != NULL) {
    munmap(output->traces_mmap, output->traces_file_size);
  }

  target_file = g_strdup_printf("%s/job-%zu-%zu", output->data_path, output->id, output->chunk_cnt);

  if ((fd = open(target_file, O_RDWR | O_CREAT, S_IRWXU)) < 0) {
    g_printf("could not acquire next chunk\n");
    return -1;
  }

  output->traces_file_size = CHUNK_SIZE * sizeof(trace_t);
  if ((ret = ftruncate(fd, output->traces_file_size)) < 0) {
    g_printf("Could not truncate file to required size of %d!\n", CHUNK_SIZE);
    return -1;
  }

  if ((output->traces_mmap = mmap(NULL, output->traces_file_size, PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED,
                                  fd, 0)) == MAP_FAILED) {
    g_printf("Could not mmap file into memory!\n");
    return -1;
  }

  g_printf("Got memory mapped region at address %p\n", output->traces_mmap);

  err = NULL;
  if (!g_close(fd, &err)) {
    g_printf("Could not close chunk\n");
    return -1;
  }

  output->curr_trace = output->traces_mmap;
  output->traces_written = 0;

  output->chunk_cnt++;
}

static void chunked_output_write(chunked_output_t *output, trace_t *sample) {
  /* g_fprintf(_fp, "%" PRIu64 ",%d,%d\n", sample.timestamp_ns, sample.channel, sample.state); */
  memcpy(output->curr_trace, sample, sizeof(trace_t));
  output->curr_trace++;
  output->traces_written++;

  if (G_UNLIKELY(output->traces_written > CHUNK_SIZE)) {
    if(acquire_next_chunk(output) < 0) {
      g_printf("Could not acquire next memory chunk at count %z\n!", output->chunk_cnt);
    }
  }
}


int chunked_output_init(chunked_output_t *output, const gchar* data_path) {
  int ret;
  size_t len;

  g_printf("Initializing chunked output module!\n");

  output->chunk_cnt = 0;
  output->id = 0; // for now just 1

  len = sizeof(gchar)*strlen(data_path) + 1;
  output->data_path = malloc(len);
  memcpy(output->data_path, data_path, len);

  output->write = (output_write_function)chunked_output_write;

  if ((ret = acquire_next_chunk(output)) < 0) {
    g_printf("Could not create initial chunk %s\n");
    return -1;
  }

  return 0;
}
