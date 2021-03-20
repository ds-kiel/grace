#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <glib.h>
#include <libsigrok/libsigrok.h>
#include <types.h>

#define MAX_PERIOD_DEVIATION 30
#define TIME_LOOP_CONSTANT 1024

/* create a sigrok instance for fx2ladw compatible devices */

typedef enum clock_state {
  WAIT, // wait for reference signal
  OFFSET, // clock has received at least one reference signal
  FREQ, // clock has received at least two reference signals
} clock_state_t;

typedef enum process_state {
  STOPPED,
  RUNNING,
  ERROR,
} process_state_t;

struct channel_mode {
  guint8 channel;
  gint8 mode;
};

typedef struct lclock {
  guint32 nom_freq; // nominal frequency
  double nom_period; // nominal period
  timestamp_t phase; // current phase of clock
  double period;

  clock_state_t state;
  timestamp_t prev_phase;

  double P;
  double I;
  double D;

  double freq; // frequency offset from nominal
  double offset; // last offset from reference clock
  double res_error; //residue error
} lclock_t;

typedef struct preprocess_instance {
  process_state_t state;
  struct sr_context *sr_cntxt;
  struct sr_session *sr_session;
  struct sr_dev_inst *fx2ladw_dvc_instc;

  guint8 channel_count;
  char active_channel_mask;
  struct channel_mode *channels;

  GAsyncQueue *trace_queue;
  GAsyncQueue *timestamp_unref_queue;
  GAsyncQueue *timestamp_ref_queue;

  lclock_t local_clock;
} preprocess_instance_t;

/* --- proto --- */
int preprocess_init(preprocess_instance_t *process, GVariant *channel_modes, GAsyncQueue *trace_queue, GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue);
int preprocess_stop_instance(preprocess_instance_t *process);

// TODO rename to something more meaninfull
gboolean preprocess_running(preprocess_instance_t* process);

static int preprocess_init_sigrok(preprocess_instance_t *process, guint32 samplerate);
static int preprocess_kill_instance(preprocess_instance_t *process);

#endif /* PREPROCESS_H */
