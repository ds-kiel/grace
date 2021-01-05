#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <glib.h>
/* create a sigrok instance for fx2ladw compatible devices */

struct channel_mode {
  guint8 channel;
  gint8 mode;
};

/* --- proto --- */
int preprocess_init(GVariant* channel_modes, GAsyncQueue *trace_queue, GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue);
int preprocess_stop_instance();
int preprocess_announce_sync();

// TODO rename to something more meaninfull
gboolean preprocess_running();

static int preprocess_init_sigrok(guint32 samplerate);
static int preprocess_kill_instance();

#endif /* PREPROCESS_H */
