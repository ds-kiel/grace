#include <output.h>

#include <stdlib.h>
#include <stdio.h>

static const gchar* output_file_name;
static FILE *fp;

#ifdef USE_CONSTANT_SIZE_BUFFER
static size_t buf_index = 0;
timestamp_t* write_buffer;

//" TODO complete output # timestamp,observer_id,node_id,pin_name,value\n" +

// should return -1 if failed
static int flush_buffer_to_log() {
    for(size_t k = 0; k < buf_index; k++) {
      timestamp_t* sample = &write_buffer[k];
      fprintf(fp, "%d,%d,%" PRIu64 "\n", sample->channel, sample->state, sample->time);
    }

    buf_index = 0;
    return 1;
}
#endif



int open_output_file(const gchar* filename, gboolean overwrite) {
  fp = fopen(filename, overwrite ? "w+" : "a+");
  fprintf(fp, "GPIO, Edge, Time\n");
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

int write_system_timestamp(char *event_name, struct timespec *timestamp) {
  flush_buffer_to_log(); // hack, write this data at some seperate section. For example header.
  fprintf(fp, "%s,%" PRIu64 "\n", event_name, ((guint64)timestamp->tv_sec*1e9)+ (guint64) timestamp->tv_nsec);
}

// TODO maybe memory manage timestamps in timestamp.h and only
int write_sample(timestamp_t sample) {
  #ifndef USE_CONSTANT_SIZE_BUFFER
  fprintf(fp, "%d,%d,%" PRIu64 "\n", sample->channel, sample->state, sample->time);
  #else
  write_buffer[buf_index] = sample;
  buf_index++;
  if(buf_index >= CONSTANT_SIZE_BUFFER_ENTRIES) {
    flush_buffer_to_log();
  }
  #endif
  return 0;
}
