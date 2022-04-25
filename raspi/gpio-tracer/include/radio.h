#ifndef RADIO_H
#define RADIO_H

#include <glib.h>

#define RADIO_TARGET_FREQUENCY 433000000

int radio_init();
int radio_deinit();
void radio_start_reception();
guint64 radio_retrieve_timestamp();

#endif /* RADIO_H */
