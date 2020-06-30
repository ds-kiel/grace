#ifndef GPIOT_H
#define GPIOT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>

#define GPIOT_DOMAIN_SOCKET "/tmp/gpiotd.socket"

typedef enum gpiot_command_type {
  GPIOT_START_RECORDING,
  GPIOT_STOP_RECORDING,
  GPIOT_GET_RESULTS, // yes or no ?
} gpiot_command_type_t;

typedef enum gpiot_devices {
  GPIOT_DEVICE_NONE,
  GPIOT_DEVICE_PIGPIO,
  GPIOT_DEVICE_SALEAE,
} gpiot_devices_t;

typedef enum gpiot_daemon_state {
  GPIOTD_IDLE,
  GPIOTD_COLLECTING,
} gpiot_daemon_state_t;

/* --------------------------------------- */
/* | channel | bcm-gpio | sigrok-channel | */
/* |       0 |    23    |              0 | */
/* |       1 |    -     |              1 | */
/* |       2 |    -     |              2 | */
/* |       3 |    -     |              3 | */
/* |       4 |    -     |              4 | */
/* |       5 |    -     |              5 | */
/* --------------------------------------- */

// Outpath has to be a null terminated string
typedef struct gpiot_start_data {
  uint8_t channel_mask;
  gpiot_devices_t device;
  char out_path[128]; // allow variable length
} gpiot_start_data_t;

// TODO split into header and payload packets. Header has to announce the size of the following
// payload packet. If tcp is used, fragmentation of the packet has to be accounted for, so fragments have
// to be buffered until the packet is complete.
typedef struct gpiot_command_header {
  gpiot_command_type_t type;
  char payload[256];
} gpiot_command_header_t;




/*Start Command [ type | channel_mask  | ---- variable length path ---] */

#endif /* GPIOT_H */
