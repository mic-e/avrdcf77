#include "dbg.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "util.h"

#ifndef NDEBUG

#define RINGBUF_SIZE 512
#define RINGBUF_PTR_MASK (RINGBUF_SIZE - 1)
static char uart_ringbuf[RINGBUF_SIZE];
static uint16_t uart_ringbuf_pos = 0;
static uint16_t uart_ringbuf_end = 0;


/**
 * Adds a single character to the ringbuf.
 *
 * Designed for usage in a FILE stream.
 *
 * Interrupt-safe.
 */
void uart_putc(char c, FILE *stream) {
	if (c == '\n') {
		uart_putc('\r', stream);
	}

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		uart_ringbuf[uart_ringbuf_end++] = c;
		uart_ringbuf_end &= RINGBUF_PTR_MASK;

		// Check for buffer overflow.
		if (uart_ringbuf_pos == uart_ringbuf_end) {
			// Advance pos (discard oldest byte from the buffer).
			uart_ringbuf_pos++;
			uart_ringbuf_pos &= RINGBUF_PTR_MASK;
		}

		// The ringbuf now has contents -> enable the TX interrupt.
		UCSR0B |= (1 << UDRIE0);
	}
}

/**
 * This ISR is enabled via UDRIE0 whenever the ringbuf has conents.
 * Even if the ringbuf is empty, it may be activated without consequence.
 * It takes a single byte from the ringbuf and feeds it to the TX.
 */
ISR(USART_UDRE_vect) {
	// Abort if the buffer is empty.
	if (uart_ringbuf_pos == uart_ringbuf_end) {
		// Disable the TX interrupt.
		UCSR0B &= ~(1 << UDRIE0);
		return;
	}

	UDR0 = uart_ringbuf[uart_ringbuf_pos++];
	uart_ringbuf_pos &= RINGBUF_PTR_MASK;
}

void dbg_init() {
	// Setup PD3 and PD4 (LED pins) as outputs.
	DDRD |= (1 << PD3) | (1 << PD4);

	// Enable UART.
	UBRR0 = (F_CPU / (16 * SERIALBAUD)) - 1;
	UCSR0B |= (1 << TXEN0);

	// Setup uart_putc as stdout.
	static FILE my_stdout = FDEV_SETUP_STREAM(uart_putc, NULL,
		_FDEV_SETUP_WRITE);

	stdout = &my_stdout;
	stderr = &my_stdout;
}

void dbg_toggle_yellow() {
	PORTD ^= (1 << PD3);
}

void dbg_toggle_red() {
	PORTD ^= (1 << PD4);
}

#endif
