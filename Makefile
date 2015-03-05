# files
SRCS=main.c util.c led.c dbg.c dcf_receiver.c dcf_processor.c monotime.c gregorian_calendar.c lcd.c time_display.c
ELF=dcf77avr.elf
HEX=dcf77avr.hex
OBJS=$(SRCS:.c=.o)
DEPS=$(SRCS:.c=.d)

# hardware
MCU=atmega328p
F_CPU=16000000
SERIALBAUD=57600

# toolchain
CC=avr-gcc
OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump
AVRSIZE=avr-size
AVRDUDE=avrdude
VIEWTTY=ttycat
TTY=$(shell ls -t /dev/ttyUSB* | head -1)
FLASHFLAGS=-c arduino -P $(TTY) -b 57600

# flags
CFLAGS=-mmcu=$(MCU) -DF_CPU=$(F_CPU) -DSERIALBAUD=$(SERIALBAUD) -MD -MP -Wall -Wextra -pedantic -g -std=c11 -Os
LDFLAGS=

.PHONY: all
all: $(HEX)

$(ELF): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	$(AVRSIZE) -C --mcu=$(MCU) $@

$(HEX): $(ELF)
	$(OBJCOPY) -j .text -j .data -O ihex $^ $@

# include -MD dependencies
-include $(DEPS)

# general rule for object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: flash
flash: $(HEX)
	$(AVRDUDE) $(FLASHFLAGS) -p $(MCU) -U flash:w:$<:i

.PHONY: run
run: flash
	$(VIEWTTY) --baud $(SERIALBAUD) $(TTY)

.PHONY: asm
asm: $(ELF)
	$(OBJDUMP) -d $(ELF)

.PHONY: clean
clean:
	rm -f $(OBJS) $(DEPS) $(ELF) $(HEX)
