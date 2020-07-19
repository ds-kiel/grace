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


typedef enum gpiot_devices {
  GPIOT_DEVICE_NONE,
  GPIOT_DEVICE_PIGPIO,
  GPIOT_DEVICE_SALEAE,
} gpiot_devices_t;

typedef enum gpiot_daemon_state {
  GPIOTD_IDLE,
  GPIOTD_COLLECTING,
} gpiot_daemon_state_t;

#endif /* GPIOT_H */
