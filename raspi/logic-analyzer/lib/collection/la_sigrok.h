#ifndef LA_SIGROK_H
#define LA_SIGROK_H

#include "../configuration.h"

/* create a sigrok instance for fx2ladw compatible devices */

/* --- proto --- */

int fx2_init_instance(test_configuration_t* configuration);
int fx2_run_instance();
int fx2_end_session();

#endif /* LA_SIGROK_H */
