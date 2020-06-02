#include <context.h>

int main(int argc, char *argv[])
{
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
  fx2_init_instance(&conf);
  fx2_run_instance();
  return 0;
}
