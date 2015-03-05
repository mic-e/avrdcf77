// Provides a simple monotonic timer that is based on the CPU clock.
// Uses the 16-bit Timer 1.

#ifndef DCF77AVR_MONOTIME_H_
#define DCF77AVR_MONOTIME_H_

#include <stdint.h>

/**
 * Holds the number of seconds since clock initialization.
 * The least significant byte holds a decimal fraction.
 * Shift left by 8 bits to get the whole number of seconds.
 *
 * The actual clock resolution is to 1/128th of a second,
 * so the least significant bit is superfluous.
 *
 * May be safely used only from within any ISR.
 * For use outside an ISR, use monotime_current_get().
 */
extern volatile uint32_t monotime_current;

/**
 * As above, but is interrupt-safe.
 */
uint32_t monotime_current_get();

/**
 * Initializes the monotinic clock and the associated timer.
 *
 * Must run with interrupts globally disabled.
 * The clock becomes active once global interrupts are enabled.
 */
void monotime_init();

#endif
