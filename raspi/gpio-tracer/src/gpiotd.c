#include "gpiot.h"

#include <collection/la_sigrok.h>
#include <collection/la_pigpio.h>
#include <configuration.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#include <unistd.h>

gpiot_daemon_state_t state = GPIOTD_IDLE;

channel_configuration_t channels[2];
test_configuration_t conf;

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
      gpiot_command_t* cmd = (gpiot_command_t*)buffer;
      switch(cmd->type) {
      case GPIOT_START_RECORDING:
        printf("Received start recording\n");
        if(state == GPIOTD_IDLE) {
          channels[0].channel = 0;
          channels[0].type = MATCH_FALLING | MATCH_RISING;
          channels[1].channel = 1;
          channels[1].type = MATCH_FALLING | MATCH_RISING;

          conf.channels = channels;
          conf.channel_count = 2;
          conf.logpath = "testrun.csv";
          conf.samplerate = 24000000;

          if(la_pigpio_init_instance(&conf) < 0) {
            printf("Could not create pigpio instance\n");
          } else {
            la_pigpio_run_instance();
            state = GPIOTD_COLLECTING;
          }
        }
        break;
      case GPIOT_STOP_RECORDING:
        if(GPIOTD_COLLECTING){
          printf("Received stop recording\n");
          la_pigpio_end_instance();
        }
        break;
      case GPIOT_GET_RESULTS:
        printf("get results\n");
      }
    }
    close(gpiotc_socket);
  }
  close(gpiotd_socket);
}
