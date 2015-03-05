#ifndef DCF77AVR_TIME_DISPLAY_H_
#define DCF77AVR_TIME_DISPLAY_H_

/**
 * Draws the current (gregorian) date to the LCD.
 */
void display_gregorian_date();

/**
 * Draws the current (gregorian) time to the LCD.
 */
void display_gregorian_time();

/**
 * Draws the current UNIX time to the LCD.
 */
void display_unix_time();

/**
 * Draws the current monotonic time to the LCD.
 */
void display_monotime();

#endif
