#include "gpiot.h"

#include <collection/la_sigrok.h>
#include <collection/la_pigpio.h>
#include <configuration.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <glib.h>

#include <unistd.h>

gpiot_daemon_state_t state = GPIOTD_IDLE;
gpiot_devices_t device = GPIOT_DEVICE_NONE;

channel_configuration_t channels[2];
test_configuration_t conf;

int main(int argc, char *argv[])
{

  GMainContext* g_context =  g_main_context_new();
  g_main_context_push_thread_default(g_context);


  // create UNIX domain socket for ipc with gpiotc
  int gpiotd_socket;
  char *buffer = malloc(sizeof(gpiot_command_header_t));

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

    printf("%d\n", g_main_context_iteration(g_context, FALSE));

    buf_size = recv(gpiotc_socket, buffer, sizeof(gpiot_command_header_t), 0);
    if(buf_size==sizeof(gpiot_command_header_t)) {
      gpiot_command_header_t* cmd = (gpiot_command_header_t*)buffer;
      switch(cmd->type) {
      case GPIOT_START_RECORDING:
        printf("Received start recording\n");
        gpiot_start_data_t start_data;
        memcpy(&start_data, cmd->payload, sizeof(gpiot_start_data_t));
        if(state == GPIOTD_IDLE) {

          channels[0].channel = 0;
          channels[0].type = MATCH_FALLING | MATCH_RISING;
          channels[1].channel = 1;
          channels[1].type = MATCH_FALLING | MATCH_RISING;
          conf.channels = channels;
          conf.channel_count = 2;
          strcpy(conf.logpath, start_data.out_path);
          conf.samplerate = 24000000;


          if(start_data.device == GPIOT_DEVICE_PIGPIO) {
            printf("Creating pigpio instance\n");
            if(la_pigpio_init_instance(&conf) < 0) {
              printf("Could not create pigpio instance\n");
            } else {
              la_pigpio_run_instance();
              state = GPIOTD_COLLECTING;
              device = GPIOT_DEVICE_PIGPIO;
            }
          } else if (start_data.device == GPIOT_DEVICE_SALEAE) {
            printf("Creating saleae instance\n");
            if(la_sigrok_init_instance(&conf) < 0) {
              printf("Could not create pigpio instance\n");
            } else {
              la_sigrok_run_instance();
              state = GPIOTD_COLLECTING;
              device = GPIOT_DEVICE_SALEAE;
            }
          } else {
            printf("unknown device. Doing nothing\n");
          }
        }
        break;
      case GPIOT_STOP_RECORDING:
        if(state == GPIOTD_COLLECTING){
          printf("Received stop recording\n");
          if (device == GPIOT_DEVICE_PIGPIO) {
            la_pigpio_end_instance();
          }
          else if(device == GPIOT_DEVICE_SALEAE) {
            la_sigrok_stop_instance();
          }
          device = GPIOT_DEVICE_NONE;
          state = GPIOTD_IDLE;
        }
        la_sigrok_session_status();

        break;
      case GPIOT_GET_RESULTS:
        printf("get results\n");
      }
    }
    close(gpiotc_socket);
  }
  close(gpiotd_socket);

  g_main_context_pop_thread_default(g_context);
}
