#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef enum match {
  //  MATCH_NONE = 0, // could be used optionally to be more explicit
  MATCH_FALLING = 1,
  MATCH_RISING = 2,
  /// ... = 4, = 8
} match_t;

#endif /* TYPES_H */
