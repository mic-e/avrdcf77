#include "lcd.h"

#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "util.h"
#include "dbg.h"

FILE lcd_stream = FDEV_SETUP_STREAM(lcd_putc, NULL, _FDEV_SETUP_WRITE);

#define LCD_COLS 16

/**
 * The current LCD line number ("cursor position").
 */
uint8_t lcd_line;

/**
 * The current LCD column number ("cursor position").
 */
uint8_t lcd_col;

// Exposed globally via the header file.
uint8_t lcd_redraw;

void (*lcd_line0)();
void (*lcd_line1)();

void lcd_set_line_functions(void (*line0)(), void (*line1)()) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		lcd_line0 = line0;
		lcd_line1 = line1;
	}
}

/**
 * Used in the above pointers to draw empty lines.
 */
void lcd_draw_empty_line() {}

// Exposed globally via the header file;

void lcd_clock() {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		_delay_us(1);
		PORTC |= (1 << PC5);
		_delay_us(1);
		PORTC &= ~(1 << PC5);
	}
}

void lcd_command(uint8_t cmd) {
	// Send the higher nibble.
	PORTC = (cmd >> 4);
	lcd_clock();
	// Send the lower nibble.
	PORTC = (cmd & 0xf);
	lcd_clock();
	// Wait for the LCD to finish.
	_delay_us(50);
}

void lcd_data(uint8_t data) {
	// Send the higher nibble.
	PORTC = (data >> 4) | (1 << 4);
	lcd_clock();
	// Send the lower nibble.
	PORTC = (data & 0xf) | (1 << 4);
	lcd_clock();
	// Wait for the LCD to finish.
	_delay_us(50);
}

void lcd_moveto(uint8_t line, uint8_t col) {
	lcd_line = line & 0x1;
	lcd_col = col;
	if (lcd_col >= LCD_COLS) {
		lcd_col = LCD_COLS;
	}

	lcd_command(1 << 7 | (lcd_line << 6) | lcd_col);
}

void lcd_init() {
	lcd_line = 0;
	lcd_col = 0;

	lcd_redraw = 1;

	lcd_line0 = lcd_draw_empty_line;
	lcd_line1 = lcd_draw_empty_line;

	// Init the LCD.
	// Configure the port.
	PORTC = 0;
	DDRC = 0x3f; // enable PC0 - PC5 as outputs.

	// Wait for the LCD to finish booting.
	_delay_ms(15);

	// Initialization instruction.
	PORTC = 0x3;

	for (uint8_t rep = 0; rep < 3; rep++) {
		// Repeat to reset and initialize from any possible state.
		lcd_clock();
		_delay_ms(6);
	}

	// Configure for 4-bit mode.
	// Unfortunately we can't control the other configuration
	// options because we don't have access to the lower data lanes.
	PORTC = 0x2;
	lcd_clock();
	_delay_ms(6);

	// Configure for 4-bit, 2-line, 5x7 mode.
	lcd_command(0x28);

	// Configure for LCD on / cursor off / blinking off.
	lcd_command(0x0c);

	// Conigure for positive cursor movement / no scrolling.
	lcd_command(0x06);

	// Clear LCD.
	lcd_command(0x01);
	_delay_ms(2);

	// Send cursor to home position.
	lcd_moveto(0, 0);
}

/**
 * Fills the remainder of the current line with the char c.
 */
void lcd_fill_remaining_line(char c) {
	while (lcd_col < LCD_COLS) {
		putc(c, lcd);
	}
}

void lcd_update() {
	if (!lcd_redraw) {
		return;
	}

	lcd_redraw = 0;

	lcd_moveto(0, 0);
	lcd_line0();
	lcd_fill_remaining_line(' ');
	lcd_moveto(1, 0);
	lcd_line1();
	lcd_fill_remaining_line(' ');
}

/**
 * Puts a char to the LCD.
 *
 * Designed for usage in a FILE stream.
 */
void lcd_putc(char c, FILE *stream) {
	UNUSED(stream);

	if (lcd_col == LCD_COLS) {
		printf("LCD line overflow in line %hd.\n", lcd_line);
		return;
	}

	// Write the char.
	lcd_data(c);

	// Advance the cursor position.
	lcd_col++;
}
