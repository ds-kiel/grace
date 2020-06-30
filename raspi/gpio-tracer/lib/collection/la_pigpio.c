#include "la_pigpio.h"

#include "../timestamp.h"
#include "../output.h"


#include <pigpio.h>
#include <stdio.h>

test_configuration_t* c_conf;

// returns -1 if pigpio instance creation failed
int la_pigpio_init_instance(test_configuration_t* configuration) {
  c_conf = configuration;
  open_output_file(c_conf->logpath);

  int ret;
  if ((ret = gpioInitialise()) == PI_INIT_FAILED) {
    printf("Could not create pigpio session code: %d.\n", ret);
    return -1;
  }
}

void isr_handler(int gpio, int level, uint32_t tick) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  timestamp_t data = {.channel = 0, .state = level, .time = ts};
  write_sample(data);
}

int la_pigpio_run_instance() {
  // listen on gpio BCM 23
  gpioSetISRFunc(23, EITHER_EDGE, 0, &isr_handler);
}

int la_pigpio_end_instance() {
  close_output_file();
  gpioTerminate();
}
