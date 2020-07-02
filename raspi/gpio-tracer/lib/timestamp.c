// https://www.man7.org/linux/man-pages/man2/clock_gettime.2.html
#include "timestamp.h"
#include <stdio.h>

#define CLOCK_TYPE CLOCK_MONOTONIC

unsigned long b_size; // block size
guint64 sample_count;

struct timespec start_time; // TODO a start time is also passed by the first datafeed header packet.
 // Use unsigned long or uint64_t? Is it better to let the program fail on compile while being explicit on how long a time capture may take
 // or let the system run on a best effort basis -> e.g. 32 bit raspberyr pi overflowing after ~70 minutes?
/* unsigned long last_time_stamps_micros; */

double seconds_per_sample;


int init_clock(guint64 samplerate, unsigned long block_size) {
  /* start_time = time(NULL); */
  clock_gettime(CLOCK_TYPE, &start_time);
  seconds_per_sample = (double)b_size/samplerate;
  b_size = block_size;
  return 1;
}

int get_timestamp(struct timespec *ts) { // this should be handled some other way
  // microsecond timestamp
  clock_gettime(CLOCK_TYPE, ts);
  ts->tv_nsec -= (b_size - sample_count)*((unsigned long)1e9*seconds_per_sample);
  return 1;
}
