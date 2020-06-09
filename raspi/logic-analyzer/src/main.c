#include <collection/la_sigrok.h>
#include <collection/la_pigpio.h>

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

// current way of stopping the program is by terminating with SIGINT. This should probably be changed to some other method later
static void sigint_handler(int sig) {
  printf("received sigint!!\n");
  la_pigpio_end_instance();
}

int main(int argc, char *argv[])
{

  signal(SIGINT, sigint_handler);
  // we want to capture channel 0 and 1 of the logic analyzer and match on every falling and rising edge
  channel_configuration_t channels[2] = {
    {.channel = 0, .type = MATCH_FALLING | MATCH_RISING},
    {.channel = 1, .type = MATCH_FALLING | MATCH_RISING}
  };
  test_configuration_t conf = {
    .channels = channels,
    .channel_count = 2,
    .samplerate = 24000000,
    .logpath = "testrun-data.csv"
  };
  /* fx2_init_instance(&conf); */
  /* fx2_run_instance(); */
  la_pigpio_init_instance(&conf);
  la_pigpio_run_instance();

  // spa spa spa spaghetti code
  sleep(30);
  la_pigpio_end_instance();

  return 0;
}
