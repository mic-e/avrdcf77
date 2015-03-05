// Provides raw bits from a DCF77 receiver at PIN DC; uses INT0.

#ifndef DCF77AVR_DCFISR_H_
#define DCF77AVR_DCFISR_H_

#include <stdint.h>

/**
 * Initializes the INT0 I/O pin and ISR.
 *
 * Must run with interrupts globally disabled.
 * The receiver interrupt will become active once interrupts have been
 * globally enabled.
 */
void dcf_receiver_init();

/**
 * Asks for a new set of minute bits.
 *
 * A new set becomes available every time a minute-end marker is received.
 *
 * The data is not guaranteed to be consistent or even contain the correct
 * number of bits.
 *
 * @result:
 *     1 if data has been available, 0 else.
 * @param minute_bits:
 *     If the result is 1, the bits received previously to the minute-end
 *     marker are stored here.
 *     They have been assembled by left-shifting; the LSB is the last bit
 *     of the minute.
 *     The most significant non-zero bit is not part of the received data;
 *     instead it acts as a marker that may be used to determine the number
 *     of received bits.
 * @param timestamp_monotime:
 *     If the result is 1, the monotime timestamp of the start of the next
 *     minute is stored here.
 *
 * This method is interrupt-safe.
 */
uint8_t dcf_poll_data(uint64_t *minute_bits, uint32_t *timestamp_monotime);

#endif
