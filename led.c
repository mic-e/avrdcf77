#include "led.h"

#include <avr/io.h>

void led_set(uint8_t value) {
	if (value) {
		PORTB |= (1 << PB5);
	} else {
		PORTB &= ~(1 << PB5);
	}
}

uint8_t led_get() {
	return PORTB & (1 << PB5);
}

void led_init() {
	led_set(0);

	// make PB5 an output.
	DDRB |= (1 << PB5);
}
