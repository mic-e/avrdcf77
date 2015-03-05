#ifndef DCF77AVR_LED_H_
#define DCF77AVR_LED_H_

#include <stdint.h>

/**
 * sets the LED on PB5.
 *
 * @param value:
 *     if 0, LED is turned off. Else, LED is turned on.
 */
void led_set(uint8_t value);

/**
 * gets the status of the LED on PB5.
 *
 * @result:
 *     non-zero if the LED is on.
 */
uint8_t led_get();

/**
 * initializes the LED on PB5.
 *
 * initial state is OFF.
 */
void led_init();

#endif
