#include "time.h"
#include "cc1101.h"

volatile uint8_t __time_seconds_packet[TIME_PACKET_LEN] = {header_burst_tx_fifo, TIME_PACKET_LEN-2, 0, 0, 0, 0};
volatile uint32_t* time_seconds = (__time_seconds_packet+2);
volatile uint32_t pps_ext_counter_period;
volatile uint32_t pps_ext_overflow_counter;
volatile uint32_t pps_int_counter_period;
volatile uint8_t pps_measured_pulses;

volatile uint32_t pps_int_counter_period;

void TIME_Reset(void) {
  *time_seconds = 0;
  pps_measured_pulses = 0;
}
