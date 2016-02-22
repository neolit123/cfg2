/*
 * cfg2
 * a simplistic configuration parser for INI like syntax in C
 *
 * author: lubomir i. ivanov (neolit123 at gmail)
 * this code is released in the public domain without warranty of any kind.
 * providing credit to the original author is recommended but not mandatory.
 *
 * utils.c:
 *	utility functions
 */

#include "defines.h"

/* local implementation of strdup() if missing on a specific C89 target */
cfg_char *cfg_strdup(const cfg_char *str)
{
	cfg_uint32 n;
	cfg_char *copy;

	if (!str)
		return NULL;
	n = strlen(str);
	copy = (cfg_char *)malloc(n + 1);
	if (copy) {
		memcpy((void *)copy, (void *)str, n);
		copy[n] = '\0';
	}
	return copy;
}

/* fast fnv-32 hash */
cfg_uint32 cfg_hash_get(const cfg_char *str)
{
	cfg_char *ptr = (cfg_char *)str;
	cfg_uint32 hash = CFG_HASH_SEED;
	if (!str)
		return hash;
	while (*ptr) {
		hash *= hash;
		hash ^= *ptr++;
	}
	return hash;
}

/* string -> number conversations */
cfg_bool cfg_value_to_bool(const cfg_char *value)
{
	if (!value)
		return CFG_FALSE;
	return strtoul(value, NULL, 2);
}

cfg_int cfg_value_to_int(const cfg_char *value)
{
	if (!value)
		return 0;
	return (cfg_int)strtol(value, NULL, 0);
}

cfg_long cfg_value_to_long(const cfg_char *value)
{
	if (!value)
		return 0;
	/* the strtoll() portability is a mess; use strtod() and cast to 64bit integer
	 * instead */
	return (cfg_long)strtod(value, NULL);
}

cfg_float cfg_value_to_float(const cfg_char *value)
{
	if (!value)
		return 0.f;
	/* older MSVC are missing strtof(); use strtod() with a cast instead() */
	return (cfg_float)strtod(value, NULL);
}

cfg_double cfg_value_to_double(const cfg_char *value)
{
	if (!value)
		return 0.0;
	return strtod(value, NULL);
}

/* number -> string conversations */
cfg_char *cfg_bool_to_value(cfg_bool number)
{
	static const char *format = "%u";
	cfg_char *buf;
	int sz;

	if (number != CFG_FALSE)
		number = CFG_TRUE;
	sz = sprintf(NULL, format, number);
	if (sz < 0)
		return NULL;
	buf = (cfg_char *)malloc(sz + 1);
	sprintf(buf, format, number);
	return buf;
}

/* number -> string conversations */
cfg_char *cfg_int_to_value(cfg_int number)
{
	static const char *format = "%li";
	cfg_char *buf;
	int sz;

	sz = sprintf(NULL, format, number);
	if (sz < 0)
		return NULL;
	buf = (cfg_char *)malloc(sz + 1);
	sprintf(buf, format, number);
	return buf;
}

cfg_char *cfg_long_to_value(cfg_long number)
{
	static const char *format = "%lli";
	cfg_char *buf;
	int sz;

	sz = sprintf(NULL, format, number);
	if (sz < 0)
		return NULL;
	buf = (cfg_char *)malloc(sz + 1);
	sprintf(buf, format, number);
	return buf;
}

cfg_char *cfg_float_to_value(cfg_float number)
{
	static const char *format = "%f";
	int sz;
	cfg_char *buf;

	sz = sprintf(NULL, format, number);
	if (sz < 0)
		return NULL;
	buf = (cfg_char *)malloc(sz + 1);
	sprintf(buf, format, number);
	return buf;
}

cfg_char *cfg_double_to_value(cfg_double number)
{
	static const char *format = "%lf";
	int sz;
	cfg_char *buf;

	sz = sprintf(NULL, format, number);
	if (sz < 0)
		return NULL;
	buf = (cfg_char *)malloc(sz + 1);
	sprintf(buf, format, number);
	return buf;
}

/* the lookup table acts both as a toupper() converter and as a shifter of any
 * [0x0 - 0xff] char in the [0x0 - 0x0f] range */
static const cfg_char hex_to_char_lookup[] = {
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
1,   2,   3,   4,   5,   6,   7,   8,   9,  10,   0,   0,   0,   0,   0,   0,
0,  11,  12,  13,  14,  15,  16,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,  11,  12,  13,  14,  15,  16,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 };

cfg_char *cfg_hex_to_char(const cfg_char *value)
{
	static const cfg_char *fname = "[cfg2] cfg_hex_to_char():";
	cfg_uint32 len, badchar_pos;
	cfg_char *buf, *src_pos, *dst_pos;
	cfg_char first, second, *value_ptr = (cfg_char *)value;

	if (!value) {
		fprintf(stderr, "%s the input value is NULL!\n", fname);
		return NULL;
	}
	len = strlen(value);
	if (len % 2 || !len) {
		fprintf(stderr, "%s input length is zero or not divisible by two!\n", fname);
		return NULL;
	}
	len >>= 1;
	buf = (cfg_char *)malloc(len + 1);
	if (!buf) {
		fprintf(stderr, "%s cannot allocate buffer of length %u!\n", fname, len + 1);
		return NULL;
	}
	for (src_pos = value_ptr, dst_pos = buf; *src_pos != '\0'; src_pos += 2, dst_pos++) {
		first = hex_to_char_lookup[(cfg_uchar)*src_pos];
		second = hex_to_char_lookup[(cfg_uchar)*(src_pos + 1)];
		if (!first || !second) {
			badchar_pos = src_pos - value;
			goto error;
		}
		first--;
		second--;
		*dst_pos = (cfg_char)((first << 4) + second);
	}
	buf[len] = '\0';
	return buf;
error:
	free(buf);
	fprintf(stderr, "%s input has bad character at position %u!\n", fname, badchar_pos);
	return NULL;
}


static const cfg_char char_to_hex_lookup[] = {
48,     49,     50,     51,     52,     53,     54,     55,     56,     57,
65,     66,     67,     68,     69,     70 };

cfg_char *cfg_char_to_hex(const cfg_char *value)
{
	static const cfg_char *fname = "[cfg2] cfg_char_to_hex():";
	cfg_uint32 len, first, second;
	cfg_uchar in_char;
	cfg_char *out, *ptr, *value_ptr = (cfg_char *)value;

	if (!value) {
		fprintf(stderr, "%s the input is NULL!\n", fname);
		return NULL;
	}
	len = strlen(value);
	if (!len) {
		fprintf(stderr, "%s the input length is zero!\n", fname);
		return NULL;
	}
	len = len * 2 + 1;
	out = (cfg_char *)malloc(len);
	if (!out) {
		fprintf(stderr, "%s cannot allocate buffer of length %u!\n", fname, len);
		return NULL;
	}
	ptr = out;
	while (*value_ptr) {
		in_char = *value_ptr;
		first = in_char >> 4;
		second = in_char - (first << 4);
		*(ptr++) = char_to_hex_lookup[first];
		*(ptr++) = char_to_hex_lookup[second];
		value_ptr++;
	}
	*ptr = '\0';
	return out;
}
