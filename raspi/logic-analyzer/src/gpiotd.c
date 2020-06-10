#include "gpiot.h"

#include <collection/la_sigrok.h>
#include <collection/la_pigpio.h>
#include <configuration.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#include <unistd.h>

gpiot_daemon_state_t state;

int main(int argc, char *argv[])
{
  // create UNIX domain socket for ipc with gpiotc
  int gpiotd_socket;
  char *buffer = malloc(sizeof(gpiot_command_t));

  struct sockaddr_un address;


  if((gpiotd_socket = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
    printf("could not bind to socket %s: %s\n", GPIOT_DOMAIN_SOCKET, strerror(errno));
    return EXIT_FAILURE;
  }
  printf("Bound to socket %s\n", GPIOT_DOMAIN_SOCKET);

  unlink(GPIOT_DOMAIN_SOCKET);
  address.sun_family = AF_LOCAL;
  strcpy(address.sun_path, GPIOT_DOMAIN_SOCKET);

  if (bind(gpiotd_socket, (struct sockaddr *)&address, sizeof(address)) != 0) {
    printf("could not bind to socket %s: %s\n", GPIOT_DOMAIN_SOCKET, strerror(errno));
    return EXIT_FAILURE;
  }

  listen(gpiotd_socket, 3);

  socklen_t addrlen = sizeof(struct sockaddr_in);
  int gpiotc_socket;
  ssize_t buf_size;
  while (1) {
    gpiotc_socket = accept(gpiotd_socket, (struct sockaddr *)&address, &addrlen);
    addrlen = sizeof(struct sockaddr_in);

    buf_size = recv(gpiotc_socket, buffer, sizeof(gpiot_command_t), 0);
    if(buf_size==sizeof(gpiot_command_t)) {
      printf("received command\n");
      gpiot_command_t* cmd = (gpiot_command_t*)buffer;
      switch(cmd->type) {
      case GPIOT_START_RECORDING:
        if(state == GPIOTD_IDLE) {
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
          la_pigpio_init_instance(&conf);
          la_pigpio_run_instance();
          state = GPIOTD_COLLECTING;
        }
        break;
      case GPIOT_STOP_RECORDING:
        la_pigpio_end_instance();
        break;
      case GPIOT_GET_RESULTS:
        printf("get results\n");
      }
    }
    close(gpiotc_socket);
  }
  close(gpiotd_socket);
}
