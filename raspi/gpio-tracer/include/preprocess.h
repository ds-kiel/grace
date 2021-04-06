#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <glib.h>
#include <libsigrok/libsigrok.h>
#include <output_module.h>
#include <types.h>

#define MAX_PERIOD_DEVIATION 30
#define TIME_LOOP_CONSTANT 1024
#define ANALYZER_FREQUENCY 8000000

/* #define ANALYZER_FREQUENCY 12000000 */

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
  timestamp_t nom_period; // nominal period

  timestamp_t phase; // current phase of clock
  timestamp_t period;

  clock_state_t state;

  // used for open loop measurements to determine frequency dev.
  timestamp_t prev_ref_phase;
  timestamp_t prev_seq;
  guint64 seq;

  timestamp_t I;

  timestamp_t freq; // current running frequency
  timestamp_t offset; // last offset from reference clock
  timestamp_t res_error; //residue error

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

  output_module_t *output;

  lclock_t local_clock;
} preprocess_instance_t;

/* --- proto --- */
int preprocess_init(preprocess_instance_t *process, output_module_t *output, GVariant *channel_modes, GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue);
int preprocess_stop_instance(preprocess_instance_t *process);

// TODO rename to something more meaninfull
gboolean preprocess_running(preprocess_instance_t* process);

static int preprocess_init_sigrok(preprocess_instance_t *process, guint32 samplerate);
static int preprocess_kill_instance(preprocess_instance_t *process);

#endif /* PREPROCESS_H */
