#include "util.h"

void print_binary_64(FILE *f, uint64_t val) {
	print_binary_32(f, (uint32_t) (val >> 32));
	print_binary_32(f, (uint32_t) (val));
}

void print_binary_32(FILE *f, uint32_t val) {
	print_binary_16(f, (uint16_t) (val >> 16));
	print_binary_16(f, (uint16_t) (val));
}

void print_binary_16(FILE *f, uint16_t val) {
	print_binary_8(f, (uint8_t) (val >> 8));
	print_binary_8(f, (uint8_t) (val));
}

void print_binary_8(FILE *f, uint8_t val) {
	uint8_t mask = (1 << 7);

	while (mask != 0) {
		if (val & mask) {
			putc('1', f);
		} else {
			putc('0', f);
		}

		mask >>= 1;
	}
}
