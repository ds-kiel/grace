#ifndef OUTPUT_H
#define OUTPUT_H

#include "timestamp.h"
#include <glib.h>

/* If USE_CONSTANT_SIZE_BUFFER is defined data will be collected in memory and only */
/* written to disk after collection has ended. If undefined write every sample directly to disk. */
// TODO remove if not needed and find out wether one can just write from another thread or sth like that
#define USE_CONSTANT_SIZE_BUFFER 1
#define CONSTANT_SIZE_BUFFER_ENTRIES 40e6

// returns -1 if file can't be opened for writing
int open_output_file(const gchar* filename, gboolean overwrite);
int close_output_file();

// open_output_file has to be called before using write_sample
int write_sample(timestamp_t sample);
int write_system_timestamp(char *event_name, struct timespec *timestamp);

#endif /* OUTPUT_H */
