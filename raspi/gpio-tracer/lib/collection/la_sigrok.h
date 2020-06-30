#ifndef LA_SIGROK_H
#define LA_SIGROK_H

#include "../configuration.h"

/* create a sigrok instance for fx2ladw compatible devices */

/* --- proto --- */

int la_sigrok_init_instance(test_configuration_t* configuration);
int la_sigrok_run_instance();
int la_sigrok_stop_instance();

#endif /* LA_SIGROK_H */
