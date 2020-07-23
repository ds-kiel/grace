#ifndef GPIOT_H
#define GPIOT_H

/* --------------------------------------- */
/* | channel | bcm-gpio | sigrok-channel | */
/* |       0 |    23    |              0 | */
/* |       1 |    -     |              1 | */
/* |       2 |    -     |              2 | */
/* |       3 |    -     |              3 | */
/* |       4 |    -     |              4 | */
/* |       5 |    -     |              5 | */
/* --------------------------------------- */

/* typedef enum return_code { */
/*   LA_SUCCESS = 0, */
/*   LA_ERROR = -1, */
/* } */

typedef enum gpiot_daemon_state {
  GPIOTD_IDLE = 1,
  GPIOTD_COLLECTING = 2,
  GPIOTD_PENDING_SYNC = 3,
} gpiot_daemon_state_t;

#endif /* GPIOT_H */
