#include "gregorian_calendar.h"

#include "dbg.h"
#include "util.h"

// Made available externally via the header file.
struct gregorian_date_time current_date_time;

void gregorian_calendar_init() {
	// Set the alert bit to warn about the wrong date.
	current_date_time.call_bit = 1;
	current_date_time.timezone = +1;
	current_date_time.timezone_change_announced = 0;
	current_date_time.time.leap_second_announced = 0;

	current_date_time.time.second = 0;
	current_date_time.time.minute = 0;
	current_date_time.time.hour = 0;
	current_date_time.date.day_of_month = 1;
	current_date_time.date.month = 1;
	current_date_time.date.year = 15;
	current_date_time.date.century = 20;
	current_date_time.date.day_of_week = 4;

	gregorian_date_calculate_unix_date(&current_date_time.date);
	gregorian_date_time_calculate_unix_time(&current_date_time);

	current_date_time.epoch_monotime = -current_date_time.unix_time;
}

void gregorian_date_time_increment(struct gregorian_date_time *datetime) {
	// Next second.
	datetime->unix_time++;
	datetime->time.second += 1;

	if (datetime->time.second < 60) {
		return;
	}

	if (datetime->time.second == 60 && datetime->time.minute == 59 &&
	    datetime->time.leap_second_announced) {
		return;
	}

	// Next minute.
	datetime->time.second = 0;
	datetime->time.minute++;

	if (datetime->time.minute < 60) {
		return;
	}

	// Next hour.
	datetime->time.minute = 0;
	if (datetime->time.leap_second_announced) {
		// The UNIX time gets decremented when leaving a leap second.
		datetime->unix_time--;
		datetime->epoch_monotime += 0x100;
		datetime->time.leap_second_announced = 0;
	}

	if (datetime->timezone_change_announced) {
		datetime->timezone_change_announced = 0;

		switch (datetime->timezone) {
		case +1:
			// We're switching to UTC+2.
			datetime->time.hour += 2;
			datetime->timezone = +2;
			break;
		case +2:
			// We're switching to UTC+1.
			// The hour remains unchanged.
			datetime->timezone = +1;
			break;
		default:
			// Undefined; just continue as before, and wait for
			// the next, correcting minute frame.
			datetime->time.hour++;
			break;
		}
	} else {
		datetime->time.hour++;
	}

	if (datetime->time.hour < 24) {
		return;
	}

	// Next day.
	datetime->time.hour -= 24;
	datetime->date.day_of_month += 1;
	datetime->date.day_of_week = (datetime->date.day_of_week + 1) % 7;
	datetime->date.unix_date += 1;

	if (datetime->date.day_of_month <=
	    gregorian_date_length_of_month(&datetime->date,
	                                    datetime->date.month)) {

		return;
	}

	// Next month.
	datetime->date.day_of_month = 1;
	datetime->date.month += 1;

	if (datetime->date.month <= 12) {
		return;
	}

	// Next year.
	datetime->date.month = 1;
	datetime->date.year += 1;

	if (datetime->date.year <= 100) {
		return;
	}

	// Next century.
	datetime->date.year = 0;
	datetime->date.century += 1;

	// Overflows of the century can not be handled correctly.
	// (due to the lack of display space).
	// Anyways, try our best by resetting it to the safe value of 4000.
	if (datetime->date.century == 100) {
		datetime->date.century = 40;

		// Don't forget to re-calculate the UNIX date and time.
		gregorian_date_calculate_unix_date(&datetime->date);
		gregorian_date_time_calculate_unix_time(datetime);
	}
}

const char *get_day_name(uint8_t day_of_week) {
	switch(day_of_week) {
	case 0:  return "So";
	case 1:  return "Mo";
	case 2:  return "Tu";
	case 3:  return "Mi";
	case 4:  return "Th";
	case 5:  return "Fr";
	case 6:  return "Sa";
	}

	return "??";
}

uint8_t gregorian_date_length_of_month(struct gregorian_date *date, uint8_t month) {
	switch(month) {
	case  1: return 31;
	case  2:
		if ((date->year & 3) == 0) {
			if (date->year != 0) {
				return 29;
			}
			if ((date->century & 3) == 0) {
				return 29;
			}
		}
		return 28;
	case  3: return 31;
	case  4: return 30;
	case  5: return 31;
	case  6: return 30;
	case  7: return 31;
	case  8: return 31;
	case  9: return 30;
	case 10: return 31;
	case 11: return 30;
	case 12: return 31;
	}

	// Unknown month; this should not have passed verification.
	return 0;
}


uint8_t gregorian_date_validate(struct gregorian_date *date) {
	// The day-of-month and day-of-week can only be validated with a known
	// century-of-cycle.

	// Validate month.
	if (date->month == 0 || date->month >= 13) {
		printf("Month is out of range: %d.\n", date->month);
		return 0;
	}

	// Validate year.
	if (date->year >= 100) {
		printf("Year is out of range: %d.\n", date->year);
		return 0;
	}

	// Validate range of day-of-week.
	// Note that DCF77 transmit the dow in a range of 1-7 (sunday == 7);
	// after this validation, the range will be a more reasonable 0-6
	// (sunday == 0).
	//
	// The actual value will be validated as part of the century-guessing.
	if (date->day_of_week > 7 || date->day_of_week == 0) {
		printf("Day of week is out of range: %d.\n", date->day_of_week);
		return 0;
	}

	if (date->day_of_week == 7) {
		date->day_of_week = 0;
	}

	// Determine the century/validate the day-of-week value.
	date->century = 20;
	while (1) {
		gregorian_date_calculate_unix_date(date);

		// The day-of-week _should_ be this:
		// 1970-01-01 was a thursday
		uint8_t unix_day_of_week = (date->unix_date + 4) % 7;
		if (unix_day_of_week == date->day_of_week) {
			// Yup, seems to fit.
			break;
		}

		// Try an other century.
		date->century++;

		if (date->century == 24) {
			// No match has been found.
			puts("Day of week not valid for any century.\n");
			return 0;
		}
	}

	// Validate the day of month.
	if ((date->day_of_month == 0) ||
	    (date->day_of_month >= gregorian_date_length_of_month(date, date->month))) {
		printf("Day of month is out of range: %d.\n", date->day_of_month);
	}

	// Clamp the century to the one we'd like.
	// If century=20, year=01, we'd prefer to read that as 2401.
	gregorian_date_clamp_timespan(date, 20, 15);

	// Re-calculate the UNIX date.
	gregorian_date_calculate_unix_date(date);

	// Everything was alright.
	return 1;
}

void gregorian_date_clamp_timespan(struct gregorian_date *date,
	uint8_t century, uint8_t year) {

	int8_t adjustment = (int8_t) century - (int8_t) date->century;

	if (year >= date->year) {
		// Find minimum k such that date->century + k * 4 >= century.
		adjustment += 3;
	} else {
		// Find minimum k such that date->century + k * 4 > century.
		adjustment += 4;
	}

	date->century += ((adjustment >> 2) << 2);

	printf("Century: %d00.\n", date->century);
}

uint8_t gregorian_time_validate(struct gregorian_time *time) {
	if (time->hour >= 24) {
		printf("Hour is out of range: %d.\n", time->hour);
		return 0;
	}

	if (time->minute >= 60) {
		printf("Minute is out of range: %d.\n", time->minute);
		return 0;
	}

	if (time->second >= 60) {
		if (time->second == 60 &&
		    time->leap_second_announced &&
		    time->minute == 59) {
			puts("Second=60 is allowed because it's a leap second.\n");
		} else {
			printf("Second is out of range: %d.\n", time->second);
			return 0;
		}
	}

	return 1;
}

void gregorian_date_calculate_unix_date(struct gregorian_date *date) {
	// The UNIX date of 2000-01-01.
	uint32_t unix_date = (uint32_t) 365 * 30 + 7; // 30 years, 7 leap years.

	// Every full gregorian cycle (400 years) has precisely 146097 days.
	uint8_t cycles = (date->century - 20) >> 2;
	unix_date += (uint32_t) cycles * ((uint32_t) 365 * 400 + 97);

	// Add the number of days for the position-in-cycle.
	switch(date->century & 3) {
	case 3:
		unix_date += (uint32_t) 365 * 100 + 24;
		// fall through.
	case 2:
		unix_date += (uint32_t) 365 * 100 + 24;
		// fall through.
	case 1:
		unix_date += (uint32_t) 365 * 100 + 25;
		// fall through.
	case 0:
		break;
	}

	// Add the number of days in the completed years in this century.
	if (date->year) {
		unix_date += ((uint32_t) 365 * (uint32_t) date->year);
		// Add leap years.
		unix_date += ((date->year + 3) >> 2);
		// If we're not at the beginning-of-cycle, year 0 is no leap
		// year.
		if ((date->century & 3) != 0) {
			unix_date -= 1;
		}
	}

	// Add the lengths of all completed months.
	for (uint8_t month = 1; month < date->month; month++) {
		unix_date += gregorian_date_length_of_month(date, month);
	}

	// Add the number of completed days in this month.
	unix_date += date->day_of_month - 1;

	// And that's it.
	date->unix_date = unix_date;
}

void gregorian_date_time_calculate_unix_time(
	struct gregorian_date_time *datetime) {

	datetime->unix_time = datetime->date.unix_date;
	datetime->unix_time *= 24;
	datetime->unix_time += datetime->time.hour - datetime->timezone;
	datetime->unix_time *= 60;
	datetime->unix_time += datetime->time.minute;
	datetime->unix_time *= 60;
	datetime->unix_time += datetime->time.second;

	// And that's it.
}
