#include "monotime.h"

#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "dbg.h"
#include "gregorian_calendar.h"
#include "lcd.h"

// Made available globally by the header.
// Holds the number of times 1/256th of a second has passed since
// monotime_init().
volatile uint32_t monotime_current;

uint32_t monotime_current_get() {
	volatile uint32_t result;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		result = monotime_current;
	}

	return result;
}

void monotime_init() {
	// set clock divider to 8.
	TCCR1B |= (1 << CS11);
	// set compare value to 15624 to achieve a clock interval of 1/128s.
	OCR1A = (F_CPU / 8 / 128) - 1;
	// enable clear-on-timer-compare.
	TCCR1B |= (1 << WGM12);
	// enable interrupt-on-timer-compare.
	TIMSK1 |= (1 << OCIE1A);

	monotime_current = 0;
}

ISR(TIMER1_COMPA_vect) {
	// Increment monotime_current twice, since this ISR triggers 128 times
	// a second.
	// Overflows do not hurt us here (perfectly defined behavior).
	monotime_current += 2;

	// If the last byte (the sub-second fraction) of the current
	// monotime is identical to the epoch's monotime, a new second
	// as started. Thus, update the datetime.
	// Don't check the last bit (since we're incrementing by two each
	// time).
	if ((monotime_current & 0xfe) ==
	    (current_date_time.epoch_monotime & 0xfe)) {
		gregorian_date_time_increment(&current_date_time);

		// Send re-draw instruction to display.
		lcd_redraw = 1;
	}
}
