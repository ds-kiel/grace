#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include "types.h"
#include <glib.h>

#define CONSTANT_SIZE_BUFFER_ENTRIES 40e6

int open_output_file(const gchar* filename, gboolean overwrite);
int postprocess_init(GAsyncQueue trace_queue);
int close_output_file();

int write_sample(trace_t sample);
int write_comment(gchar *comment);

#endif /* POSTPROCESS_H */
