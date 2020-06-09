#ifndef LA_PIGPIO_H
#define LA_PIGPIO_H

#include "../configuration.h"

int la_pigpio_init_instance(test_configuration_t* configuration);
int la_pigpio_run_instance();
int la_pigpio_end_instance();

#endif /* LA_SIGROK_H */
