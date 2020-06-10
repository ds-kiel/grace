#include "gpiot.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static void usage() {
      printf("Missing arguments. usage:\n");
      printf("   gpiotc [--start|--stop]\n");
}

int main(int argc, char *argv[])
{
  gpiot_command_t commando;

  if (argc < 2) {
    usage();
    return EXIT_FAILURE;
  }

  if(!strcmp(argv[1], "--start")) {
    commando.type = GPIOT_START_RECORDING;
  } else if (!strcmp(argv[1], "--stop")) {
    commando.type = GPIOT_STOP_RECORDING;
  } else {
    printf("unknown argument %s\n", argv[1]);
    usage();
    return EXIT_FAILURE;
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

  send(gpiotd_socket, &commando, sizeof(gpiot_command_t), 0);
}
