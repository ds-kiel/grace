#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <glib.h>


// TODO rename
// used in case the incoming data stream has to be buffered first
typedef struct timestamp {
  guint8 channel; // channel for which the state is recorded
  char state;
  guint64 time; // in nanoseconds
} timestamp_t;


/* extern guint64 sample_count; */

// return -1 on error
int init_clock(guint64 samplerate, unsigned long block_size);
void next_system_timestamp();
int get_timestamp(struct timespec *ts);
#endif /* TIMESTAMP_H */
