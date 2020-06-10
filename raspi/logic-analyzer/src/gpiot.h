#ifndef GPIOT_H
#define GPIOT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>

#define GPIOT_DOMAIN_SOCKET "/var/run/gpiotd.socket"

typedef enum gpiot_command_type {
  GPIOT_START_RECORDING,
  GPIOT_STOP_RECORDING,
  GPIOT_GET_RESULTS, // yes or no ?
} gpiot_command_type_t;

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
typedef struct gpiot_start_data {
  char channel_mask;
} gpiot_start_data_t;

typedef struct gpiot_command {
  gpiot_command_type_t type;
  char payload[16];
} gpiot_command_t;

#endif /* GPIOT_H */
