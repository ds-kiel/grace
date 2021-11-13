#ifndef RADIO_H
#define RADIO_H

#include <glib.h>

#define RADIO_TARGET_FREQUENCY 433000000

GAsyncQueue *reference_timestamp_queue;

int radio_init(GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue);
int radio_deinit();
void radio_start_reception();
guint64 radio_retrieve_timestamp();

#endif /* RADIO_H */
