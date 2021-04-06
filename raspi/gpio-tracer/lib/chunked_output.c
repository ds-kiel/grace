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

typedef struct chunked_output {
  write_function write;

  gchar *data_path;
  GAsyncQueue *trace_queue;
  size_t chunk_cnt;
  jobid id;

  trace_t *traces_mmap;
  trace_t *curr_trace;
  size_t traces_written;
} chunked_output_t;


chunked_output_t* chunked_output_new() {
  return malloc(sizeof(chunked_output_t));
}


int chunked_output_deinit(chunked_output_t *process) {
  /* flush_buffer_to_log(); */
  free(process->data_path);

  return 0;
}

// returns -1 on failure
int acquire_next_chunk(chunked_output_t *process) {
  int ret;
  GError *err;
  int fd;
  gchar *target_file;
  size_t file_size;

  target_file = g_strdup_printf("%s/job-%z-%z", process->data_path, process->id, process->chunk_cnt);

  if ((fd = g_open(target_file, O_WRONLY | O_CREAT, S_IRWXU)) < 0) {
    g_printf("could not acquire next chunk\n");
    return -1;
  }

  file_size = CHUNK_SIZE * sizeof(trace_t);
  if ((ret = ftruncate(fd, file_size)) < 0) {
    g_printf("Could not truncate file to required size of %d!\n", CHUNK_SIZE);
    return -1;
  }

  if ((process->traces_mmap = mmap(NULL, file_size, PROT_WRITE, MAP_SHARED, fd, 0)) == NULL) {
    g_printf("Could not mmap file into memory!\n");
    return -1;
  }

  if (!g_close(fd, &err)) {
    g_printf("Could not close chunk\n");
    return -1;
  }

  process->curr_trace = process->traces_mmap;
  process->traces_written = 0;

  process->chunk_cnt++;
}

int write_sample(chunked_output_t *process, trace_t *sample) {
  /* g_fprintf(_fp, "%" PRIu64 ",%d,%d\n", sample.timestamp_ns, sample.channel, sample.state); */
  memcpy(process->curr_trace, sample, sizeof(trace_t));
  process->curr_trace++;
  process->traces_written++;

  if (process->traces_written > CHUNK_SIZE) {
    if(acquire_next_chunk(process) < 0) {
      g_printf("Could not acquire next memory chunk at count %z\n!", process->chunk_cnt);
      return -1;
    }
  }

  return 0;
}


int chunked_output_init(chunked_output_t *process, const gchar* data_path) {
  int ret;
  size_t len;

  g_printf("Initializing chunked output module!\n");

  process->chunk_cnt = 0;
  process->id = 0; // for now just 1

  len = sizeof(gchar)*strlen(data_path) + 1;
  process->data_path = malloc(len);
  memcpy(process->data_path, data_path, len);

  if ((ret = acquire_next_chunk(process)) < 0) {
    g_printf("Could not create initial chunk %s\n");
    return -1;
  }

  return 0;
}
