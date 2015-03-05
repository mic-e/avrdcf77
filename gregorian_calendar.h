// Methods for validating, handling and printing gregorian dates, as
// transmitted by DCF77 and displayed on the LCD.

// For reasons of "why the heck would I even implement or test this?",
// all methods have undefined behavior for dates earlier than 2000-01-01.

#ifndef DCF77AVR_GREGORIAN_CALENDAR_H_
#define DCF77AVR_GREGORIAN_CALENDAR_H_

#include <stdint.h>

struct gregorian_date {
	uint8_t day_of_week;
	uint8_t day_of_month;
	uint8_t month;
	uint8_t year;         // The year of century.
	uint8_t century;

	uint32_t unix_date;   // Days since 1970-01-01.
};

struct gregorian_time {
	uint8_t second;
	uint8_t minute;
	uint8_t hour;

	// Set in the hour that has 3601 seconds.
	uint8_t leap_second_announced;
};

struct gregorian_date_time {
	struct gregorian_date date;
	struct gregorian_time time;

	// Half-hours are not supported (but who uses those...).
	int8_t timezone;

	// Set in the hour before each time zone change.
	uint8_t timezone_change_announced;

	// Seconds since 1970-01-01 00:00:00 UTC, ignoring leap seconds (duh.).
	// This code will survive y2k38 :P
	uint64_t unix_time;

	// The precise monotime at 1970-01-01 00:00:00 UTC.
	int64_t epoch_monotime;

	// Set in the case of abnormal transmitter operation.
	uint8_t call_bit;
};

/**
 * Contains the current date-time.
 * Is updated by the DCF processor whenever a correct minute has been
 * received;
 * Is incremented during the interrupts of the monotonic timer.
 */
extern struct gregorian_date_time current_date_time;

/**
 * Returns a two-letter name of the day of week.
 */
const char *get_day_name(uint8_t day_of_week);

/**
 * Increments the datetime by one second.
 *
 * Respects leap second and timezone change announcements.
 */
void gregorian_date_time_increment(struct gregorian_date_time *datetime);

/**
 * Returns the length of the given month (January = 1).
 * Performs no validations.
 */
uint8_t gregorian_date_length_of_month(struct gregorian_date *date,
	uint8_t month);

/**
 * This method performs no validations.
 */
void gregorian_date_calculate_unix_date(struct gregorian_date *date);

/**
 * This method performs no validations; you must call calculate_unix_date
 * beforehand.
 */
void gregorian_date_time_calculate_unix_time(
	struct gregorian_date_time *datetime);

/**
 * Performs range checks and calculates the century and unix date.
 * Goes as far as validating day_of_week and validating
 * day_of_month in a leap year-correct manner, and day_of_week.
 *
 * Note that, due to the fact that one cycle of the gregorian calendar
 * (400 years) is, by chance, divisible by 7, there is an unique
 * association between day_of_week and century_of_cycle.
 *
 * @returns 0 on failure, 1 on success.
 */
uint8_t gregorian_date_validate(struct gregorian_date *date);

/**
 * Initializes the gregorian calendar; sets the current date time to some
 * more or less resonable default value.
 */
void gregorian_calendar_init();

/**
 * Only the last two bits of the century may be determined from the
 * day_of_week; everything else is guesswork.
 *
 * This method adjusts the century with multiples of 4 such that the
 * smallest possible century/year is the one given in the arguments.
 */
void gregorian_date_clamp_timespan(struct gregorian_date *date,
	uint8_t century, uint8_t year);

/**
 * Performs range checks (including leap seconds).
 */
uint8_t gregorian_time_validate(struct gregorian_time *time);

#endif
