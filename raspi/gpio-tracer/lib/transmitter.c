#include <transmitter.h>

#include <glib.h>
#include <pigpio.h>

#define TRANSMITTER_GPIO_PIN 23


// returns the nanosecond timestamp at which the pulse was transmitted
guint64 transmitter_send_pulse()
{
   for(int i = 0; i < TRANSMITTER_WARMUP_TRANSFER_PULSES; i++) {
    gpioWrite(TRANSMITTER_GPIO_PIN, 1);
    g_usleep(TRANSMITTER_WARMUP_HIGH_LENGTH);
    gpioWrite(TRANSMITTER_GPIO_PIN, 0);
    g_usleep(TRANSMITTER_WARMUP_LOW_LENGTH);
  }

  struct timespec pulse_time;
  clock_gettime(CLOCK_REALTIME, &pulse_time);

  gpioWrite(TRANSMITTER_GPIO_PIN, 1);
  g_usleep(TRANSMITTER_SYNC_LENGTH);
  gpioWrite(TRANSMITTER_GPIO_PIN, 0);

  return pulse_time.tv_sec + pulse_time.tv_nsec * 1e9;
}
