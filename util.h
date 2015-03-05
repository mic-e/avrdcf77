#ifndef DCF77AVR_UTIL_H_
#define DCF77AVR_UTIL_H_

#include <stdint.h>
#include <stdio.h>

#define UNUSED(X) ((void)(X))
#define BIT(X, POS) (((uint64_t) (X) & ((uint64_t) 1 << (POS))) ? 1 : 0)

void print_binary_64(FILE *f, uint64_t val);
void print_binary_32(FILE *f, uint32_t val);
void print_binary_16(FILE *f, uint16_t val);
void print_binary_8(FILE *f, uint8_t val);

#endif
