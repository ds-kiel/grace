#ifndef LA_SIGROK_H
#define LA_SIGROK_H

#include <glib.h>
/* create a sigrok instance for fx2ladw compatible devices */

#define PPS_PULSE_LENGTH 30*1e6 // in Nanoseconds e.g. one pulse is 30 Milliseconds long
#define PPS_PULSE_TOLERANCE  5*1e6// Divergence in Nanoseconds from pulse length that still identifies as pps signal

struct channel_mode {
  guint8 channel;
  gint8 mode;
};

/* --- proto --- */
int la_sigrok_init_instance(guint64 samplerate, const gchar* logpath, GVariant* channel_modes);
int la_sigrok_run_instance();
int la_sigrok_stop_instance();

#endif /* LA_SIGROK_H */
