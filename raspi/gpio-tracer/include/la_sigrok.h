#ifndef LA_SIGROK_H
#define LA_SIGROK_H

#include <glib.h>
/* create a sigrok instance for fx2ladw compatible devices */

struct channel_mode {
  guint8 channel;
  gint8 mode;
};

enum action {
  LA_SIGROK_ACTION_START,
  LA_SIGROK_ACTION_STOP,
  LA_SIGROK_ACTION_TIMESTAMP,
};

/* --- proto --- */
int la_sigrok_init_instance(guint32 samplerate, const gchar* logpath, GVariant* channel_modes);
int la_sigrok_run_instance(gboolean wait_sync);
int la_sigrok_stop_instance(gboolean wait_sync);
// TODO rename to something more meaninfull
gboolean la_sigrok_get_sync_state(); // returns TRUE if device is waiting for a sync pulse

static int la_sigrok_do_stop_instance();

// only has an effect if sync_pulses are used
/* void la_sigrok_set_expected_action(enum action expected_action); */


#endif /* LA_SIGROK_H */
