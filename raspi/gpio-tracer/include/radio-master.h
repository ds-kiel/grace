#ifndef RADIO_MASTER_H
#define RADIO_MASTER_H

#include <glib.h>
#include <radio.h>

#define MASTER_TIMESTAMP_PERIOD 2 // period after which a new reference timestamp should be emitted

int radio_master_init();

#endif /* RADIO_MASTER_H */
