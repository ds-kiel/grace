#ifndef OUTPUT_MODULE_H
#define OUTPUT_MODULE_H

#include "types.h"
#include <glib.h>

struct output_module;

typedef void (*output_write_function)(struct output_module *process, trace_t *sample);

typedef struct output_module {
  output_write_function write;
} output_module_t;


#endif /* OUTPUT_MODULE_H */
