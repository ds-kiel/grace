#ifndef LA_PIGPIO_H
#define LA_PIGPIO_H

#include <glib.h>

int la_pigpio_init_instance(const gchar *logpath);
int la_pigpio_run_instance();
int la_pigpio_stop_instance();

#endif /* LA_SIGROK_H */
