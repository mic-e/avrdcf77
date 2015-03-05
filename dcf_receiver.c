#include "dcf_receiver.h"

#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "dbg.h"
#include "led.h"
#include "monotime.h"
#include "util.h"

/**
 * Is set to the received minute bits as part of the INT0 ISR whenever a
 * minute-end marker has been received.
 *
 * To be used via dcf_poll_data.
 */
volatile uint64_t dcf_minute_bits = 0;

/**
 * Is set to the timestamp of the minute-end-mark (start of the next minute)
 * as part of the INT0 ISR whenever a minute-end marker has been received.
 */
volatile uint32_t dcf_timestamp_monotime = 0;

/**
 * Guaranteed to be set to 1 by the INT0 ISR if and only if new values have
 * been written to dcf_minute_bits and dcf_timestamp_monotime.
 */
volatile uint8_t dcf_data_ready = 0;

void dcf_receiver_init() {
	// configure PD2 (the INT0 PIN) as a tri-state input.
	DDRD &= ~(1 << PD2);
	PORTD &= ~(1 << PD2);

	// enable INTO on rising and falling edge.
	EICRA = 1 << ISC00;
	EIMSK = 1 << INT0;
}

uint8_t dcf_poll_data(uint64_t *minute_bits, uint32_t *timestamp_monotime) {
	if (!dcf_data_ready) {
		return 0;
	}

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		*minute_bits = dcf_minute_bits;
		*timestamp_monotime = dcf_timestamp_monotime;

		dcf_data_ready = 0;
	}

	return 1;
}

ISR(INT0_vect) {
	// Edge type (rising or falling).
	uint8_t status = PIND & (1 << PD2);

	// Stores the monotime of the previous interrupt.
	static uint32_t monotime_previous = 0;
	// Calculate time since the previous interrupt (= pos/neg duty cycle).
	uint32_t duty_cycle = monotime_current - monotime_previous;
	monotime_previous = monotime_current;

	led_set(status);

	// Contains all bits from the current minute.
	// This is filled by left-shifting over the course of the minute.
	// If no minute is currently being processed, the variable is zero.
	// Otherwise, there's always a leading '1' bit that is not part of the
	// actual received data.
	static uint64_t minute_bits = 0;

	if (status) {
		// Rising edge; analyze length of the negative duty cycle:
		//
		//  0.7s - 1.0s: regular second
		//  1.7s - 2.0s: start of new minute

		if ((duty_cycle >= 179) && (duty_cycle <= 256)) {
			// It was just a regular second; everything is alright.
		} else if ((duty_cycle >= 435) && (duty_cycle <= 512)) {
			// Alright; this minute is done.
			// Time to pass the accumulated bits on for analyzing.
			// (this includes checking whether there are any/the
			//  right number of bits and whether they make the
			//  least bit of sense).
			dcf_minute_bits = minute_bits;
			dcf_timestamp_monotime = monotime_current;
			minute_bits = 0;
			dcf_data_ready = 1;
		} else {
			// This is an illegal duty cycle; the signal has
			// been corrupted.

			// Better luck next minute.
			minute_bits = 0;
			dbg_toggle_red();
		}
	} else {
		// Falling edge (bit has been received).
		if (minute_bits & ((uint64_t) 1 << 63)) {
			// Something is awfully wrong here. Maybe we missed
			// the minute-end marker.

			// Start a new minute.
			minute_bits = 0;
			dbg_toggle_red();
		}

		if (minute_bits == 0) {
			// Start a new minute.
			minute_bits = 1;
		}

		// Analyze length of the positive duty cycle:
		//
		//  0.07s - 0.13s: bit 0
		//  0.17s - 0.23s: bit 1

		if ((duty_cycle >= 10) && (duty_cycle <= 33)) {
			// We have received a "0" bit.
			minute_bits = minute_bits << 1;
			minute_bits |= 0;
		} else if ((duty_cycle >= 44) && (duty_cycle <= 59)) {
			// We have received a "1" bit.
			minute_bits = minute_bits << 1;
			minute_bits |= 1;
		} else {
			// Illegal positive duty cycle length; the signal is
			// corrupted.
			//
			// Better luck next minute.

			minute_bits = 0;
			dbg_toggle_red();
		}
	}
}
