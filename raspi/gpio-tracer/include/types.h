#ifndef TYPES_H
#define TYPES_H

#include <time.h>
#include <sys/time.h>
#include <glib.h>

struct trace {
  guint8 channel; // channel for which the state is recorded
  gint8 state;
  guint64 timestamp_ns; // timestamp in nanoseconds
};
#define TRACE_SIZE (sizeof(guint8) + sizeof(gint8) + sizeof(guint64))

struct precision_time {
  guint64 clk;
  guint64 acc;
};

#define G_VARIANT_TIMESTAMP_PAIR "(tt)"


#endif /* TYPES_H */
