// Conditionally provides functions for flashing LEDs, and links printf to
// the UART through a ringbuffer.

#ifndef DCF77AVR_DBG_H_
#define DCF77AVR_DBG_H_

#ifndef NDEBUG

#include <stdio.h>

#include <avr/pgmspace.h>

/**
 * Initializes the debugging LEDs and printf stdout.
 */
void dbg_init();

/**
 * Guess what.
 */
void dbg_toggle_yellow();

/**
 * Guess what.
 */
void dbg_toggle_red();

#define printf(format, ...) printf_P(PSTR(format), __VA_ARGS__)
#define puts(str) fputs_P(PSTR(str), stdout)

#else

#define dbg_init(...) do {} while (0)
#define dbg_toggle_yellow(...) do {} while (0)
#define dbg_toggle_red(...) do {} while (0)

#define printf(...) do {} while (0)
#define puts(...) do {} while (0)
#define putc(...) do {} while (0)

#endif

#endif
