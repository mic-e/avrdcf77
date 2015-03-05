#include "dcf_processor.h"

#include <util/atomic.h>

#include "dbg.h"
#include "gregorian_calendar.h"
#include "util.h"

/**
 * Finds the index of the highest bit that is actually '1'.
 */
uint8_t find_index_of_highest_bit(uint64_t *val) {
	// Check val byte-by-byte, starting with the most significant one.
	// AVR-GCC uses little endian, so that's byte #sizeof(*val) - 1.
	// This would not be very beautiful as a for loop.
	uint8_t byteindex = sizeof(*val);
	while (1) {
		byteindex -= 1;

		uint8_t current_byte = ((uint8_t *) val)[byteindex];
		if (current_byte == 0) {
			if (byteindex == 0) {
				// val has no bits set... unfortunately (?) C
				// can't simply 'raise ValueError()'.

				// *shrugs*
				return 0;
			}

			continue;
		}

		// We've found a byte that has a bit set.
		uint8_t result = byteindex * 8;

		// Now to find out which one...
		while (1) {
			current_byte >>= 1;

			if (current_byte) {
				result++;
			} else {
				return result;
			}
		}
	}
}

/**
 * Calculates the parity of a certain range of bits in the given word.
 */
uint8_t parity(uint64_t *word, uint8_t start, uint8_t end) {
	uint8_t result = 0;

	uint64_t mask = (1 << start);
	while (start != end) {
		if (*word & mask) {
			result = !result;
		}

		start += 1;
		mask <<= 1;
	}

	return result;
}

/**
 * Verifies the basic parities in the given minute bits.
 * Invoked by dcf_try_process.
 *
 * @returns
 *     1 on success, 0 on failure.
 */
uint8_t dcf_verify_parities(uint64_t *minute_bits) {
	if (BIT(*minute_bits, 41) == BIT(*minute_bits, 40)) {
		// The CET and CEST bits always need to have opposite values.
		puts("CET and CEST bits don't have opposite values.\n");
		return 0;
	}

	if (BIT(*minute_bits, 38) != 1) {
		// The start-of-encoded-time bit must be always 1.
		puts("Start-of-encoded-time bit is not 1.\n");
		return 0;
	}

	if (parity(minute_bits, 30, 38) != 0) {
		// Odd parity over minutes
		puts("Minute bits (30 to 37) have odd parity.\n");
		return 0;
	}

	if (parity(minute_bits, 23, 30) != 0) {
		// Odd parity over hours
		puts("Hour bits (23 to 29) have odd parity.\n");
		return 0;
	}

	if (parity(minute_bits, 0, 23) != 0) {
		// Odd parity over date
		puts("Date bits (0 to 22) have odd parity.\n");
		return 0;
	}

	// no errors have been found.
	return 1;
}

/**
 * Reads a single bit of data.
 */
inline uint8_t read_bcd_1(uint64_t *data, uint8_t pos) {
	return BIT(*data, pos);
}

/**
 * Reads a single 2-bit binary-coded digit from data,
 * from bits #pos and #pos-1.
 *
 * In case of an error, sets error=1.
 */
inline uint8_t read_bcd_2(uint64_t *data, uint8_t pos) {
	return
		BIT(*data, pos - 0) * 1 +
		BIT(*data, pos - 1) * 2;
}

/**
 * Reads a single 3-bit binary-coded digit from data,
 * from bits #pos, #pos-1 and #pos-2.
 */
inline uint8_t read_bcd_3(uint64_t *data, uint8_t pos) {
	return
		BIT(*data, pos - 0) * 1 +
		BIT(*data, pos - 1) * 2 +
		BIT(*data, pos - 2) * 4;
}

/**
 * Reads a single 4-bit binary-coded digit from data,
 * from bits #pos, #pos-1, #pos-2 and #pos-3.
 *
 * If the digit is >= 10, sets *error=1 and returns 0.
 */
inline uint8_t read_bcd_4(uint64_t *data, uint8_t pos, uint8_t *error) {
	uint8_t result =
		BIT(*data, pos - 0) * 1 +
		BIT(*data, pos - 1) * 2 +
		BIT(*data, pos - 2) * 4 +
		BIT(*data, pos - 3) * 8;

	if (result >= 10) {
		*error = 1;
		return 0;
	} else {
		return result;
	}
}

/**
 * Called by dcf_process; one instance of dcf_process might call this multiple
 * times, with different parameters.
 *
 * @param minute_bits
 *     It is guarnateed that minute_bits has at least 44 bits. A possible leap
 *     second bit has been removed from the end.
 * @param timestamp_monotime
 *     As for dcf_process.
 * @param has_leap_second
 *     True if a leap_second bit was removed from the end.
 *
 * @returns
 *     On failure, 0; on success, 1.
 */
uint8_t dcf_try_process(uint64_t *minute_bits, uint32_t *timestamp_monotime,
	uint8_t has_leap_second) {

	// verify basic parities
	if (dcf_verify_parities(minute_bits) == 0) {
		puts("Parity verification failed.\n");
		return 0;
	}

	uint8_t error = 0;

	struct gregorian_date_time datetime;

	datetime.call_bit = BIT(*minute_bits, 43);
	if (datetime.call_bit) {
		puts("Warning: Abnormal transmitter operation!\n");
	}

	datetime.timezone_change_announced = BIT(*minute_bits, 42);
	if (BIT(*minute_bits, 40)) {
		// CET
		datetime.timezone = +1;
	} else {
		// CEST
		datetime.timezone = +2;
	}

	datetime.time.leap_second_announced = BIT(*minute_bits, 39);

	datetime.time.second =       0;

	datetime.time.minute =       1 * read_bcd_4(minute_bits, 37, &error) +
	                            10 * read_bcd_3(minute_bits, 33);

	datetime.time.hour =         1 * read_bcd_4(minute_bits, 29, &error) +
	                            10 * read_bcd_2(minute_bits, 25);

	datetime.date.day_of_month = 1 * read_bcd_4(minute_bits, 22, &error) +
	                            10 * read_bcd_2(minute_bits, 18);

	datetime.date.day_of_week =  1 * read_bcd_3(minute_bits, 16);

	datetime.date.month =        1 * read_bcd_4(minute_bits, 13, &error) +
	                            10 * read_bcd_1(minute_bits,  9);

	datetime.date.year =         1 * read_bcd_4(minute_bits,  8, &error) +
	                            10 * read_bcd_4(minute_bits,  4, &error);

	printf("%hd %02hd-%02hd-%02hd %02hd:%02hd\n",
		datetime.date.day_of_week,
		datetime.date.year,
		datetime.date.month,
		datetime.date.day_of_month,
		datetime.time.hour,
		datetime.time.minute);


	if (error) {
		puts("One of the binary-coded digits was >= 10.\n");
		return 0;
	}

	if (!gregorian_time_validate(&datetime.time)) {
		return 0;
	}

	if (!gregorian_date_validate(&datetime.date)) {
		return 0;
	}

	// Check whether the leap second information is consistent.
	if (has_leap_second) {
		if (!datetime.time.leap_second_announced) {
			puts("Minute has 60 bits, but no leap second was "
			       "announced.\n");
			return 0;
		}
		if (!datetime.time.minute == 59) {
			puts("Minute has 60 bits, but only the last minute "
			       "of the hour may have a leap second.\n");
			return 0;
		}
	} else {
		if (datetime.time.leap_second_announced &&
		    datetime.time.minute == 59) {
			puts("A leap second was expected.\n");
			return 0;
		}
	}

	// It looks like - against all odds - we've received a valid word.

	// The given record describes the minute that has just passed;
	// We need to increment it by one minute by causing the seconds
	// to overflow.
	if (has_leap_second) {
		datetime.time.second = 60;
	} else {
		datetime.time.second = 59;
	}
	gregorian_date_time_increment(&datetime);

	// Finally, calculate the UNIX time.
	gregorian_date_time_calculate_unix_time(&datetime);
	printf("Unix time: %ld.\n", datetime.unix_time);

	// Time to update the clock accordingly.
	datetime.epoch_monotime = (int64_t) *timestamp_monotime -
	                          (int64_t) (datetime.unix_time << 8);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		current_date_time = datetime;
	}

	return 1;
}

uint8_t dcf_process(uint64_t *minute_bits, uint32_t *timestamp_monotime) {
	puts("Decoding new word: ");
	print_binary_64(stdout, *minute_bits);
	putc('\n', stdout);

	// Count the number of bits.
	uint8_t bit_count = find_index_of_highest_bit(minute_bits);

	// We only need 44 bits; the first 14 bits contain encrypted garbage.
	if (bit_count < 44) {
		printf("Not enough bits (%d/44).\n", bit_count);
		return 0;
	}

	if (bit_count < 60) {
		// Try to verify this second as a regular second.
		if (dcf_try_process(minute_bits, timestamp_monotime, 0)) {
			return 1;
		}

		// The processing failed, but maybe that's because it's a
		// minute with a leap second.

		puts("Not valid as a regular minute.\n"
		     "Attempting to parse it as a minute containing a leap "
		             "second...\n");

		if (bit_count < 45) {
			// But we have too little information to check if it
			// actually is.
			puts("Bit count is only 44.\n");
			return 0;
		}

		// Fall through to below.
	} else {
		puts("Minute has 60 bits; must contain a leap second.\n");
		// This is a minute with a leap second... or at least it
		// claims to be. Fall through to below.
	}

	if (BIT(*minute_bits, 0) != 0) {
		// Minutes containing a leap second end in a 0. Pretender.
		puts("Cannot be a leap second: Doesn't end in a 0.\n");
		return 0;
	}

	// Remove the last bit, and try to process it... though I still can't
	// believe that I've actually encountered a leap second here...
	*minute_bits >>= 1;
	return dcf_try_process(minute_bits, timestamp_monotime, 1);
}
