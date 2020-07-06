#include "output.h"

#include <stdlib.h>
#include <stdio.h>


static const gchar* output_file_name;
static FILE *fp;

#ifdef USE_CONSTANT_SIZE_BUFFER
static size_t buf_index = 0;
timestamp_t* write_buffer;

// TODO add semaphore synchronisation

// should return -1 if failed
static int flush_buffer_to_log() {
    for(size_t k = 0; k < buf_index; k++) {
      timestamp_t* sample = &write_buffer[k];
      fprintf(fp, "%lu %lu %d\n", sample->time.tv_sec, sample->time.tv_nsec, sample->state);
    }
    buf_index = 0;
    return 1;
}
#endif



int open_output_file(const gchar* filename) {
  // even if we write into buffer, already acquire write stream on file
  fp = fopen(filename, "a+");

  if(fp == NULL) return -1;

  #ifdef USE_CONSTANT_SIZE_BUFFER
  write_buffer = (timestamp_t* ) malloc(sizeof(timestamp_t)*CONSTANT_SIZE_BUFFER_ENTRIES);
  #endif
  return 0;
}


int close_output_file() {
  #ifdef USE_CONSTANT_SIZE_BUFFER
  flush_buffer_to_log();
  #endif
  fclose(fp);
  free(write_buffer);


  return 0;
}


// TODO maybe memory manage timestamps in timestamp.h and only
int write_sample(timestamp_t sample) {
  #ifndef USE_CONSTANT_SIZE_BUFFER
  fprintf(fp, "%lu%lu %d\n", sample->time.tv_sec, sample.time->tv_nsec, sample->state);
  #else
  write_buffer[buf_index] = sample;
  buf_index++;
  if(buf_index >= CONSTANT_SIZE_BUFFER_ENTRIES) {
    flush_buffer_to_log();
  }
  #endif
  return 0;
}
