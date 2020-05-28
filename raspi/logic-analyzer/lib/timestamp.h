#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <time.h>
#include "configuration.h"

// TODO rename
// used in case the incoming data stream has to be buffered first
typedef struct timestamp {
  char state;
  time_t time;
} timestamp_t;

/* time_t determine_timestamp(); */

#endif /* TIMESTAMP_H */
