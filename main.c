#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "dcf_receiver.h"
#include "dcf_processor.h"
#include "dbg.h"
#include "gregorian_calendar.h"
#include "lcd.h"
#include "led.h"
#include "monotime.h"
#include "time_display.h"

int main() {
	dbg_init();
	gregorian_calendar_init();
	led_init();
	dcf_receiver_init();
	monotime_init();
	lcd_init();

	lcd_set_line_functions(display_gregorian_date, display_gregorian_time);

	sei();

	puts("Initialization completed.\n");

	while (1) {
		// Check whether new minute-bits are available.

		uint64_t minute_bits;
		uint32_t timestamp_monotime;
		uint8_t poll_result = dcf_poll_data(&minute_bits,
		                                    &timestamp_monotime);

		if (poll_result) {
			if (!dcf_process(&minute_bits, &timestamp_monotime)) {
				// The data was corrupted.
				dbg_toggle_red();
				puts("Decoding failure.\n");
			} else {
				dbg_toggle_yellow();
				puts("Decoding success.\n");
			}
		}

		// Re-draw the LCD (if necessary).
		lcd_update();
	}
}
