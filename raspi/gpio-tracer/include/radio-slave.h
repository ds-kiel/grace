#ifndef RADIO_SLAVE_H
#define RADIO_SLAVE_H

#include <glib.h>
#include <radio.h>

#define MASTER_TIMESTAMP_PERIOD 5 // period after which a new reference timestamp should be emitted

int radio_master_init();

#endif /* RADIO_SLAVE_H */
