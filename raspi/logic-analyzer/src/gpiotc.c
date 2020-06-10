#include "gpiot.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[])
{
  gpio_command* commando;


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

  send(gpiotd_socket, &cmd, sizeof(gpiot_command_t), 0);
}
