#include "gpiot.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static void usage() {
      printf("Missing arguments. usage:\n");
      printf("   gpiotc [--start <output-path> [--device <pigpio|saleae>] | --stop]\n");
}

int main(int argc, char *argv[])
{
  gpiot_command_header_t command;
  gpiot_devices_t device;

  if (argc < 2) {
    usage();
    return EXIT_FAILURE;
  }

  gpiot_start_data_t start_data;
  if(!strcmp(argv[1], "--start") && argc > 2) {

    command.type = GPIOT_START_RECORDING;
    size_t path_length = strlen(argv[2]);
    strcpy(start_data.out_path, argv[2]);
    printf("%s\n", argv[2]);
    memcpy(command.payload, &start_data, sizeof(gpiot_start_data_t));
    printf("%lu", path_length);
  } else if (!strcmp(argv[1], "--stop")) {
    command.type = GPIOT_STOP_RECORDING;
  } else {
    printf("unknown or missing arguments %s\n", argv[1]);
    usage();
    return EXIT_FAILURE;
  }

  if (command.type==GPIOT_START_RECORDING) {
    if(argc > 3 && !strcmp(argv[3], "--device")) {
      if (argc > 4) {
        if (!strcmp(argv[4], "pigpio")) {
          start_data.device = GPIOT_DEVICE_PIGPIO;
        } else if (!strcmp(argv[4], "saleae")) {
          start_data.device = GPIOT_DEVICE_SALEAE;
        } else {
          usage();
        }
      } else {
        usage();
      }
    } else {
      start_data.device = GPIOT_DEVICE_PIGPIO;
    }

    memcpy(command.payload, &start_data, sizeof(gpiot_start_data_t));
  }


  // connect to gpiotd UNIX domain socket
  int gpiotd_socket;
  struct sockaddr_un address;
  int size;

  if((gpiotd_socket=socket(PF_LOCAL, SOCK_STREAM, 0)) < 1) {
    printf("Could not create socket\n");
    return EXIT_FAILURE;
  }

  address.sun_family = AF_LOCAL;
  strcpy(address.sun_path, GPIOT_DOMAIN_SOCKET);

  if (connect(gpiotd_socket, (struct sockaddr*) &address, sizeof(address)) < 0) {
    printf("Could not connect to socket %s: %s\n", GPIOT_DOMAIN_SOCKET, strerror(errno));
    return EXIT_FAILURE;
  }
  printf("Connected to socket %s\n", GPIOT_DOMAIN_SOCKET);

  send(gpiotd_socket, &command, sizeof(gpiot_command_header_t), 0);
}
