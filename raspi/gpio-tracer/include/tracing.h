#ifndef TRACING_H
#define TRACING_H

#include <glib.h>
#include <output_module.h>
#include <types.h>
#include <fx2.h>

#define ANALYZER_FREQUENCY 8000000

typedef enum clock_state {
  WAIT, // wait for reference signal
  OFFSET, // clock has received at least one reference signal
  FREQ, // clock has received at least two reference signals
} clock_state_t;

typedef enum process_state {
  STOPPED,
  RUNNING,
} process_state_t;

enum channel_state {
  SAMPLE_NONE = 0,
  SAMPLE_RISING = 1,
  SAMPLE_FALLING = 2,
  SAMPLE_ALL = SAMPLE_RISING | SAMPLE_FALLING,
  SAMPLE_RADIO = 4
};

enum TRACER_VENDOR_COMMANDS {
  VC_EPSTAT = 0xB1,
  VC_START_SAMP = 0xB2,
  VC_STOP_SAMP,
  VC_SET_DELAY,
  VC_GET_DELAY,
};

struct channel_configuration {
    enum channel_state ch1;
    enum channel_state ch2;
    enum channel_state ch3;
    enum channel_state ch4;
    enum channel_state ch5;
    enum channel_state ch6;
    enum channel_state ch7;
    enum channel_state ch8;
};

// TODO These declarations should be hidden from the user
union __channel_configuration {
  struct channel_configuration conf;
  enum channel_state conf_arr[8];
};


struct lclock {
  clock_state_t state;

  gint64 nom_freq; // nominal frequency
  /* struct precision_time curr_t; */
  guint64 freq;

  guint64 nom_frequency;
  guint64 adjusted_frequency;

  /* struct precision_time prev_ref; */
  guint64 closed_seq;
  guint64 prev_time;

  guint64 free_seq_cont;
  guint64 free_seq;

  gint64 res_error;
  gint64 offset_adj;
  gint64 offset; // last offset from reference clock

  /* guint64 freq_offset; */
  gint64 freq_offset;
};

typedef struct tracing_instance {
  process_state_t state;

  char active_channels_mask;
  union __channel_configuration chan_conf;

  GAsyncQueue *trace_queue;
  GAsyncQueue *timestamp_unref_queue;
  GAsyncQueue *timestamp_ref_queue;

  output_module_t *output;

  struct fx2_device_manager fx2_manager;
  struct fx2_bulk_transfer_config transfer_cnfg;

  struct lclock local_clock;
} tracing_instance_t;

/* --- proto --- */
int tracing_init(tracing_instance_t *process, output_module_t *output, GAsyncQueue *timestamp_unref_queue, GAsyncQueue *timestamp_ref_queue);
int tracing_start(tracing_instance_t *process, struct channel_configuration chan_conf);
int tracing_stop(tracing_instance_t *process);

void print_free_running_clock_time(tracing_instance_t *process);

// TODO rename to something more meaninfull
gboolean tracing_running(tracing_instance_t* process);

#endif /* TRACING_H */
