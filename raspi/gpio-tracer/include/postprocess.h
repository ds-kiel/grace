#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include "types.h"
#include <glib.h>

int postprocess_init(const gchar* logpath, GAsyncQueue* trace_queue, GAsyncQueue* timestamp_ref_queue);

// TODO should be file local
int close_output_file();
int open_output_file(const gchar* filename, gboolean overwrite);

int write_sample(trace_t sample);
int write_comment(gchar *comment);

#endif /* POSTPROCESS_H */
