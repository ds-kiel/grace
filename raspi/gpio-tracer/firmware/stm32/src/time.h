#ifndef __TIME_H__
#define __TIME_H__

#include <stdint.h>

#define TIME_PACKET_LEN 6

extern volatile uint8_t __time_seconds_packet[];
extern volatile uint32_t* time_seconds;
extern volatile uint32_t pps_ext_counter_period;
extern volatile uint32_t pps_ext_overflow_counter;
extern volatile uint32_t pps_int_counter_period;
extern volatile uint8_t pps_measured_pulses;

void TIME_Reset(void);

#endif /* __TIME_H__ */
