#include <transmitter.h>

#include <glib.h>
#include <pigpio.h>

#define TRANSMITTER_GPIO_PIN 23

// returns the nanosecond timestamp at which the pulse was transmitted
guint64 transmitter_send_pulse()
{
  if (gpioInitialise() < 0)
    return 0;

  gpioSetMode(TRANSMITTER_GPIO_PIN, PI_OUTPUT);

  for(int i = 0; i < TRANSMITTER_WARMUP_TRANSFER_PULSES; i++) {
    gpioWrite(TRANSMITTER_GPIO_PIN, 1);
    g_usleep(TRANSMITTER_WARMUP_HIGH_LENGTH);
    gpioWrite(TRANSMITTER_GPIO_PIN, 0);
    g_usleep(TRANSMITTER_WARMUP_LOW_LENGTH);
  }


  // save time at beginning of pulse. TODO this probably adds a small delay again. Find out how big this is
  struct timespec pulse_time;
  clock_gettime(CLOCK_REALTIME, &pulse_time);

  for(int i = 0; i < TRANSMITTER_SYNC_PULSES; i++) {
    gpioWrite(TRANSMITTER_GPIO_PIN, 1);
    g_usleep(TRANSMITTER_SYNC_LENGTH);
    gpioWrite(TRANSMITTER_GPIO_PIN, 0);
    g_usleep(TRANSMITTER_SYNC_LENGTH);
  }

  return pulse_time.tv_sec + pulse_time.tv_nsec * 1e9;
}
