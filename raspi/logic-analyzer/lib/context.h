#ifndef CONTEXT_H
#define CONTEXT_H

#include "configuration.h"

/* create a sigrok instance for fx2ladw compatible devices */

/* --- proto --- */

int fx2_init_instance(test_configuration_t configuration);
int fx2_run_instance();


#endif /* CONTEXT_H */
