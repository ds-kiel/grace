#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <stdint.h>
#include <stddef.h>

typedef enum match {
  //  MATCH_NONE = 0, // could be used optionally to be more explicit
  MATCH_FALLING = 1,
  MATCH_RISING = 2,
  /// ... = 4, = 8
} match_t;

typedef struct channel_configuration {
  uint8_t channel;
  match_t type; // use FALLING | RISING for detecting both (edges)
} channel_configuration_t;

 typedef struct test_configuration {
  channel_configuration_t* channels; // pointer to null terminated array of channel configurations
  size_t channel_count; // use GList, GArray or something else?
  uint64_t samplerate; // TODO check wether this type is 64bit on all platforms
  char logpath[256]; // filepath for the output log file
} test_configuration_t;

#endif /* CONFIGURATION_H */
