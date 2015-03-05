#ifndef DCF77AVR_DCF_PROCESSOR_H_
#define DCF77AVR_DCF_PROCESSOR_H_

#include <stdint.h>

/**
 * Tries to process the received minute bits and timestamps.
 *
 * On success, the clock is adjusted accordingly.
 *
 * @returns
 *     On error, 0. On success, 1.
 */
uint8_t dcf_process(uint64_t *minute_bits, uint32_t *timestamp_monotime);

#endif
