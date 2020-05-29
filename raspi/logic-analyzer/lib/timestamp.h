#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <time.h>
#include <sys/time.h>

#include "configuration.h"

typedef unsigned long micro_precision_timestamp_t;

// TODO rename
// used in case the incoming data stream has to be buffered first
typedef struct timestamp {
  uint8_t channel; // channel for which the state is recorded
  char state;
  micro_precision_timestamp_t time;
} timestamp_t;


extern uint32_t sample_count;

// return -1 on error
int init_clock(test_configuration_t* configuration, unsigned long block_size);
void next_system_timestamp();
micro_precision_timestamp_t get_timestamp();
#endif /* TIMESTAMP_H */
