#ifndef DCF77AVR_LCD_H_
#define DCF77AVR_LCD_H_

#include <stdio.h>
#include <stdint.h>

extern FILE lcd_stream;
#define lcd (&lcd_stream)

/**
 * Initializes the LCD. Interrupts should be disabled during this call.
 */
void lcd_init();

/**
 * Sets the line-drawing functions of the LCD.
 *
 * Interrupt-safe.
 */
void lcd_set_line_functions(void (*line0)(), void (*line1)());

/**
 * Puts a char to the LCD.
 *
 * Designed for usage in a FILE stream.
 *
 * Interrupt-safe.
 */
void lcd_putc(char c, FILE *stream);

/**
 * Set this variable to '1' to trigger re-drawing the LCD.
 */
extern uint8_t lcd_redraw;

/**
 * This function is called to draw the first line of the LCD.
 */
extern void (*lcd_line0)();

/**
 * This function is called to draw the second line of the LCD.
 */
extern void (*lcd_line1)();

/**
 * The default LCD drawing function; fills the line with spaces.
 */
void lcd_draw_empty_line();

/**
 * Re-draws both lines of the LCD using the functions pointed at by
 * lcd_line0 and lcd_line1.
 * Only runs if lcd_redraw is set to '1'.
 */
void lcd_update();

#endif
