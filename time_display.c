#include "time_display.h"

#include "lcd.h"
#include "gregorian_calendar.h"
#include "monotime.h"

void display_gregorian_time() {
	fprintf(lcd, "%02hd:%02hd:%02hd UTC%+03hd",
	        current_date_time.time.hour,
	        current_date_time.time.minute,
	        current_date_time.time.second,
	        current_date_time.timezone);

	if (current_date_time.timezone_change_announced) {
		switch (current_date_time.time.second & 3) {
		case 0:
			putc('-', lcd);
			break;
		case 1:
			putc('/', lcd);
			break;
		case 2:
			putc('|', lcd);
			break;
		case 3:
			putc('\\', lcd);
			break;
		}
	}
}

void display_gregorian_date() {
	if ((current_date_time.call_bit) &&
	    (current_date_time.time.second & 1)) {
		return;
	}

	fprintf(lcd, "%s %02hd%02hd-%02hd-%02hd",
	        get_day_name(current_date_time.date.day_of_week),
	        current_date_time.date.century,
	        current_date_time.date.year,
	        current_date_time.date.month,
	        current_date_time.date.day_of_month);

	if ((current_date_time.time.leap_second_announced) &&
	    (current_date_time.time.second & 1)) {
		putc(' ', lcd);
		putc('L', lcd);
		putc('P', lcd);
	}
}

void display_unix_time() {
	fprintf(lcd, "UNIX: %ld",
	        (int32_t) current_date_time.unix_time);
}

void display_monotime() {
	fprintf(lcd, "mono: %08lx",
	        monotime_current_get());
}
