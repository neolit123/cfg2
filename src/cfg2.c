/*
 * cfg2
 * a simplistic configuration parser for INI like syntax in C
 *
 * author: lubomir i. ivanov (neolit123 at gmail)
 * this code is released in the public domain without warranty of any kind.
 * providing credit to the original author is recommended but not mandatory.
 *
 * cfg2.c:
 *	this file holds the library definitions
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <string.h>
#include "cfg2.h"

#define CFG_SET_RETURN_STATUS(st, _status) \
	{ st->status = _status; return _status; }

#define CFG_SET_STATUS(st, _status) \
	{ st->status = _status; }

#define CFG_CHECK_ST_RETURN(st, _fname, _ret) \
	if (!st) { \
		fprintf(stderr, "[cfg2] %s(): %s\n", _fname, "the cfg_t pointer cannot bet NULL!"); \
		return _ret; \
	}

/* local implementation of strdup() if missing on a specific C89 target */
cfg_char *cfg_strdup(cfg_char *str)
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

cfg_status_t cfg_cache_clear(cfg_t *st)
{
	CFG_CHECK_ST_RETURN(st, "cfg_cache_clear", CFG_ERROR_NULL_PTR);
	if (st->cache_size && st->cache)
		memset((void *)st->cache, 0, st->cache_size * sizeof(cfg_entry_t *));
	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_status_t cfg_cache_size_set(cfg_t *st, cfg_uint32 size)
{
	int diff;

	CFG_CHECK_ST_RETURN(st, "cfg_cache_size_set", CFG_ERROR_NULL_PTR);
	if (st->init != CFG_TRUE)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_INIT);
	/* check if we are setting the buffer to zero length */
	if (!size) {
		if (!st->cache)
			CFG_SET_RETURN_STATUS(st, CFG_ERROR_NULL_PTR);
		free(st->cache);
		st->cache = NULL;
	} else {
		st->cache = (cfg_entry_t **)realloc(st->cache, size * sizeof(cfg_entry_t *));
		if (!st->cache)
			CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
		/* if the new buffer is larger, fill the extra indexes with zeroes */
		if (size > st->cache_size) {
			diff = size - st->cache_size;
			memset((void *)st->cache, 0, diff * sizeof(cfg_entry_t *));
		}
	}
	st->cache_size = size;
	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

static cfg_status_t cfg_init(cfg_t *st)
{
	CFG_CHECK_ST_RETURN(st, "cfg_init", CFG_ERROR_NULL_PTR);

	st->entry = NULL;
	st->section = NULL;
	st->verbose = 0;
	st->status = CFG_STATUS_OK;
	st->nentries = 0;
	st->nsections = 0;
	st->cache = NULL;
	st->cache_size = CFG_CACHE_SIZE;
	st->init = CFG_TRUE;
	st->separator_key_value = CFG_SEPARATOR_KEY_VALUE;
	st->separator_section = CFG_SEPARATOR_SECTION;
	st->comment_char1 = CFG_COMMENT_CHAR1;
	st->comment_char2 = CFG_COMMENT_CHAR2;
	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_status_t cfg_verbose_set(cfg_t *st, cfg_uint32 level)
{
	CFG_CHECK_ST_RETURN(st, "cfg_verbose_set", CFG_ERROR_NULL_PTR);
	st->verbose = level;
	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_status_t cfg_status_get(cfg_t *st)
{
	CFG_CHECK_ST_RETURN(st, "cfg_status_get", CFG_ERROR_NULL_PTR);
	return st->status;
}

cfg_t *cfg_alloc(cfg_bool init)
{
	cfg_t *st = (cfg_t *)calloc(1, sizeof(cfg_t));
	if (!st) {
		fprintf(stderr, "[cfg2] cfg_alloc(): cannot calloc() a cfg_t object!\n");
		return NULL;
	}
	if (init)
		cfg_init(st);
	return st;
}

/* fast fnv-32 hash */
cfg_uint32 cfg_hash_get(cfg_char *str)
{
	cfg_uint32 hash = CFG_HASH_SEED;
	if (!str)
		return hash;
	while (*str) {
		hash *= hash;
		hash ^= *str++;
	}
	return hash;
}

#define CFG_UNESCAPE_CHECK_QUOTE() \
	if (quote && st->verbose > 0) \
		fprintf(stderr, "%s: WARNING: quote not closed at line %d\n", fname, line); \
	quote = CFG_FALSE;

/* unescape all special characters (like \n) in a string */
static void cfg_unescape(cfg_t *st, cfg_char *buf, cfg_uint32 buf_sz, cfg_uint32 *keys, cfg_uint32 *sections)
{
	static const cfg_char *fname = "[cfg2] cfg_unescape()";
	cfg_uint32 line = 0;
	cfg_char *src, *dest, last_char = 0;
	cfg_bool escape = CFG_FALSE;
	cfg_bool quote = CFG_FALSE;
	cfg_bool line_eq_sign = CFG_FALSE;
	cfg_bool multiline = CFG_FALSE;
	cfg_bool section_line = CFG_FALSE;
	*keys = 0;
	*sections = 0;

	for (src = dest = buf; src < buf + buf_sz; src++) {
		/* convert separators to spaces, if found */
		if (*src == st->separator_section || *src == st->separator_key_value) {
			*src = ' ';
			continue;
		}

		/* convert CR to LF */
		if (*src == '\r')
			*src = '\n';

		/* start of a line */
		if (last_char == '\n' || src == buf) {
			line++;
			if (!multiline)
				line_eq_sign = CFG_FALSE;
			/* skip empty lines */
			while (*src == '\n' || *src == ' ' || *src == '\t') {
				if (*src == '\n') {
					line++;
					line_eq_sign = CFG_FALSE;
				}
				src++;
			}
			/* skip comment lines */
			while (*src == st->comment_char1 || *src == st->comment_char2) {
				line++;
				line_eq_sign = CFG_FALSE;
				while (*src != '\n')
					src++;
				src++;
			}
		}

		last_char = *src;
		*dest = *src;
		/* handle escaped characters */
		if (escape) {
			escape = CFG_FALSE;
			switch (*dest) {
			case 'n':
				*dest = '\n';
			case '\\':
				dest++;
				continue;
			case ' ':
			case '\n':
				multiline = CFG_TRUE;
				continue;
			}
		/* handle key/value/section separators */
		} else {
			switch (*dest) {
			case ' ':
			case '\t':
				if (!quote)
					continue;
				break;
			case '"':
				quote = !quote;
				continue;
			case '=':
				CFG_UNESCAPE_CHECK_QUOTE();
				line_eq_sign = CFG_TRUE;
				(*keys)++;
				*dest = st->separator_key_value;
				dest++;
				continue;
			case '\n':
				multiline = CFG_FALSE;
				if (!section_line) {
					if (!line_eq_sign && !quote) {
						if (st->verbose > 0)
							fprintf(stderr, "%s: WARNING: no equal sign at line %d\n", fname, line);
						continue;
					}
					CFG_UNESCAPE_CHECK_QUOTE();
					*dest = st->separator_key_value;
					dest++;
				}
				section_line = CFG_FALSE;
				continue;
			case '[':
				section_line = CFG_TRUE;
				(*sections)++;
			case ']':
				CFG_UNESCAPE_CHECK_QUOTE();
				*dest = st->separator_section;
				dest++;
				continue;
			}
		}
		escape = CFG_FALSE;
		if (*dest == '\\') {
			escape = CFG_TRUE;
			continue;
		}
		dest++;
	}
	*dest = '\0';
	if (st->verbose > 1)
		fprintf(stderr, "%s:\n%s\n", fname, buf);
}

cfg_entry_t *cfg_entry_nth(cfg_t *st, cfg_uint32 n)
{
	CFG_CHECK_ST_RETURN(st, "cfg_entry_nth", NULL);
	if (n > st->nentries - 1 || !st->nentries) {
		CFG_SET_STATUS(st, CFG_ERROR_ENTRY_NOT_FOUND);
		return NULL;
	}
	return &(st->entry[n]);
}

cfg_entry_t *cfg_entry_get(cfg_t *st, cfg_char *section, cfg_char *key)
{
	cfg_uint32 section_hash, key_hash, i;
	cfg_entry_t *entry;

	CFG_CHECK_ST_RETURN(st, "cfg_entry_get", NULL);
	if (!key) {
		CFG_SET_STATUS(st, CFG_ERROR_NULL_KEY);
		return NULL;
	}
	if (!st->nentries) {
		CFG_SET_STATUS(st, CFG_ERROR_NO_ENTRIES);
		return NULL;
	}

	section_hash = section == CFG_ROOT_SECTION ? CFG_ROOT_SECTION_HASH :
		cfg_hash_get(section);
	key_hash = cfg_hash_get(key);

	/* check for value in cache first */
	if (st->cache_size > 0) {
		for (i = 0; i < st->cache_size; i++) {
			if (!st->cache[i])
				break;
			if (section_hash != st->cache[i]->section_hash)
				continue;
			if (key_hash == st->cache[i]->key_hash) {
			    CFG_SET_STATUS(st, CFG_STATUS_OK);
				return st->cache[i];
			}
		}
	}

	for (i = 0; i < st->nentries; i++) {
		entry = &st->entry[i];
		if (section_hash != entry->section_hash)
			continue;
		if (key_hash == entry->key_hash) {
			cfg_cache_entry_add(st, entry);
			CFG_SET_STATUS(st, CFG_STATUS_OK);
			return entry;
		}
	}
	CFG_SET_STATUS(st, CFG_ERROR_ENTRY_NOT_FOUND);
	return NULL;
}

cfg_entry_t *cfg_root_entry_get(cfg_t *st, cfg_char *key)
{
	return cfg_entry_get(st, CFG_ROOT_SECTION, key);
}

cfg_status_t cfg_cache_entry_add(cfg_t *st, cfg_entry_t *entry)
{
	CFG_CHECK_ST_RETURN(st, "cfg_cache_entry_add", CFG_ERROR_NULL_PTR);
	if (!entry)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_NULL_ENTRY);
	if (!st->cache_size)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_CACHE_SIZE);

	/* lets add to the cache */
	if (st->cache_size > 1) {
		memmove((void *)(st->cache + 1), (void *)st->cache,
		        (st->cache_size - 1) * sizeof(cfg_entry_t *));
	}
	st->cache[0] = entry;
	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_char *cfg_value_get(cfg_t *st, cfg_char *section, cfg_char *key)
{
	cfg_entry_t *entry = cfg_entry_get(st, section, key);
	return entry ? entry->value : NULL;
}

cfg_char *cfg_root_value_get(cfg_t *st, cfg_char *key)
{
	return cfg_value_get(st, CFG_ROOT_SECTION, key);
}

/* string -> number conversations */
cfg_bool cfg_value_to_bool(cfg_char *value)
{
	if (!value)
		return CFG_FALSE;
	return strtoul(value, NULL, 2);
}

cfg_int cfg_value_to_int(cfg_char *value)
{
	if (!value)
		return 0;
	return (cfg_int)strtol(value, NULL, 0);
}

cfg_long cfg_value_to_long(cfg_char *value)
{
	if (!value)
		return 0;
	/* the strtoll() portability is a mess; use strtod() and cast to 64bit integer
	 * instead */
	return (cfg_long)strtod(value, NULL);
}

cfg_float cfg_value_to_float(cfg_char *value)
{
	if (!value)
		return 0.f;
	/* older MSVC are missing strtof(); use strtod() with a cast instead() */
	return (cfg_float)strtod(value, NULL);
}

cfg_double cfg_value_to_double(cfg_char *value)
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

cfg_char *cfg_hex_to_char(cfg_t *st, cfg_char *value)
{
	static const cfg_char *fname = "[cfg2] cfg_hex_to_char():";
	cfg_uint32 len, len2, badchar_pos;
	cfg_char *buf, *src_pos, *dst_pos;
	cfg_char first, second;

	if (!value) {
		if (st && st->verbose > 0)
			fprintf(stderr, "%s the input value is NULL!\n", fname);
		return NULL;
	}

	if (st && st->verbose > 0)
		fprintf(stderr, "%s input value: %s\n", fname, value);

	len = strlen(value);
	if (len % 2 || !len) {
		if (st && st->verbose > 0)
			fprintf(stderr, "%s input length is zero or not divisible by two!\n", fname);
		return NULL;
	}
	len2 = len / 2;
	buf = (cfg_char *)malloc(len2 + 1);
	if (!buf) {
		if (st && st->verbose > 0)
			fprintf(stderr, "%s cannot allocate buffer of length %u!\n", fname, len);
		return NULL;
	}
	for (src_pos = value, dst_pos = buf; *src_pos != '\0'; src_pos += 2, dst_pos++) {
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
	buf[len2] = '\0';
	if (st && st->verbose > 0)
		fprintf(stderr, "%s result: %s\n", fname, buf);
	return buf;
error:
	free(buf);
	if (st && st->verbose > 0)
		fprintf(stderr, "%s input has bad character at position %u!\n", fname, badchar_pos);
	return NULL;
}


static const cfg_char char_to_hex_lookup[] = {
48,     49,     50,     51,     52,     53,     54,     55,     56,     57,
65,     66,     67,     68,     69,     70 };

cfg_char *cfg_char_to_hex(cfg_t *st, cfg_char *value)
{
	static const cfg_char *fname = "[cfg2] cfg_char_to_hex():";
	cfg_uint32 len, first, second;
	cfg_uchar in_char;
	cfg_char *out, *ptr;

	if (!value) {
		if (st && st->verbose > 0)
			fprintf(stderr, "%s the input is NULL!\n", fname);
		return NULL;
	}

	if (st && st->verbose > 0)
		fprintf(stderr, "%s input value: %s\n", fname, value);

	len = strlen(value);
	if (!len) {
		if (st && st->verbose > 0)
			fprintf(stderr, "%s the input length is zero!\n", fname);
		out = (cfg_char *)malloc(1);
		out[0] = '\0';
		return out;
	}

	out = (cfg_char *)malloc(len * 2 + 1);
	ptr = out;
	while (*value) {
		in_char = *value;
		first = in_char >> 4;
		second = in_char - (first << 4);
		*(ptr++) = char_to_hex_lookup[first];
		*(ptr++) = char_to_hex_lookup[second];
		value++;
	}
	*ptr = '\0';

	if (st && st->verbose > 0)
		fprintf(stderr, "%s result: %s\n", fname, out);
	return out;
}

cfg_status_t cfg_entry_value_set(cfg_t *st, cfg_entry_t *entry, cfg_char *value)
{
	CFG_CHECK_ST_RETURN(st, "cfg_entry_value_set", CFG_ERROR_NULL_PTR);
	if (!entry || !value)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_NULL_PTR);
	free(entry->value);
	entry->value = cfg_strdup(value);
	if (!entry->value)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

/* should be only called if a key is really missing */
static cfg_status_t cfg_key_add(cfg_t *st, cfg_char *section, cfg_char *key, cfg_char *value)
{
	cfg_uint32 i;
	cfg_entry_t *entry;

	st->entry = (cfg_entry_t *)realloc(st->entry, (st->nentries + 1) * sizeof(cfg_entry_t));
	if (!st->entry)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
	entry = &st->entry[st->nentries];
	entry->key = cfg_strdup(key);
	entry->value = cfg_strdup(value);
	entry->key_hash = cfg_hash_get(key);
	cfg_cache_entry_add(st, entry);
	st->nentries++;

	/* handle new section creation */
	if(section == CFG_ROOT_SECTION)	{
		entry->section_hash = CFG_ROOT_SECTION_HASH;
		CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
	} else {
		entry->section_hash = cfg_hash_get(section);
	}
	for (i = 0; i < st->nsections; i++) {
		if (entry->section_hash == cfg_hash_get(st->section[i]))
			CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
	}
	st->section = (cfg_char **)realloc(st->section, (st->nsections + 1) * sizeof(cfg_char *));
	if (!st->section)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
	st->section[st->nsections] = cfg_strdup(section);
	st->nsections++;
	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_status_t cfg_value_set(cfg_t *st, cfg_char *section, cfg_char *key, cfg_char *value, cfg_bool add)
{
	cfg_uint32 i;
	cfg_uint32 key_hash, section_hash;
	cfg_entry_t *entry;

	CFG_CHECK_ST_RETURN(st, "cfg_value_set", CFG_ERROR_NULL_PTR);

	if (!value)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_NULL_PTR);

	if (!key)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_NULL_KEY);

	if (st->nentries == 0)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_NO_ENTRIES);

	key_hash = cfg_hash_get(key);
	section_hash = section == CFG_ROOT_SECTION ? CFG_ROOT_SECTION_HASH : cfg_hash_get(section);

	for (i = 0; i < st->nentries; i++) {
		entry = &st->entry[i];
		if (section_hash != entry->section_hash)
			continue;
		if (key_hash == entry->key_hash) {
			cfg_cache_entry_add(st, entry);
			return cfg_entry_value_set(st, entry, value);
		}
	}

	/* key is missing. create it */
	if (add)
		return cfg_key_add(st, section, key, value);
	CFG_SET_RETURN_STATUS(st, CFG_ERROR_ENTRY_NOT_FOUND);
}

cfg_status_t cfg_root_value_set(cfg_t *st, cfg_char *key, cfg_char *value, cfg_bool add)
{
	return cfg_value_set(st, CFG_ROOT_SECTION, key, value, add);
}

cfg_status_t cfg_entry_delete(cfg_t *st, cfg_entry_t *entry)
{
	cfg_uint32 index;

	CFG_CHECK_ST_RETURN(st, "cfg_entry_delete", CFG_ERROR_NULL_PTR);
	if (!entry)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_NULL_ENTRY);

	free(entry->key);
	free(entry->value);

	index = entry - &st->entry[0];
	if (index < st->nentries - 1)
		memcpy((void *)&st->entry[index], (void *)&st->entry[index + 1], (st->nentries - index - 1) * sizeof(cfg_entry_t));

	st->nentries--;
	st->entry = (cfg_entry_t *)realloc(st->entry, st->nentries * sizeof(cfg_entry_t));
	if (!st->entry)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);

	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_status_t cfg_section_delete(cfg_t *st, cfg_char *section)
{
	cfg_entry_t *entry;
	cfg_status_t ret;
	cfg_uint32 section_hash, section_hash_input, i;

	CFG_CHECK_ST_RETURN(st, "cfg_section_delete", CFG_ERROR_NULL_PTR);

	if (section == CFG_ROOT_SECTION) {
		section_hash_input = CFG_ROOT_SECTION_HASH;
	} else {
		section_hash_input = cfg_hash_get(section);
		for (i = 0; i < st->nsections; i++) {
			section_hash = cfg_hash_get(st->section[i]);
			if (section_hash == section_hash_input) {
				free(st->section[i]);
				memcpy((void *)&st->section[i], (void *)&st->section[i + 1], (st->nsections - i - 1) * sizeof(cfg_char *));
				st->nsections--;
				st->section = (cfg_char **)realloc(st->section, st->nsections * sizeof(cfg_char *));
				if (!st->section)
					CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
				break;
			}
		}
	}

	for (i = 0; i < st->nentries; i++) {
		/* cfg_entry_delete() updates 'nentries' */
		if (i > st->nentries - 1)
			break;
		entry = &st->entry[i];
		if (entry->section_hash == section_hash_input) {
			ret = cfg_entry_delete(st, entry);
			if (ret != CFG_STATUS_OK)
				return ret;
			i--;
		}
	}

	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

static cfg_status_t cfg_free_memory(cfg_t *st)
{
	cfg_uint32 i;

	if (!st->entry && st->nentries)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_NO_ENTRIES);

	for (i = 0; i < st->nentries; i++) {
		free(st->entry[i].key);
		free(st->entry[i].value);
	}
	free(st->entry);
	st->entry = NULL;
	st->nentries = 0;

	for (i = 0; i < st->nsections; i++) {
		free(st->section[i]);
		st->section[i] = NULL;
	}
	free(st->section);
	st->section = NULL;
	st->nsections = 0;

	free(st->cache);
	st->cache = NULL;

	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_status_t cfg_clear(cfg_t *st)
{
	CFG_CHECK_ST_RETURN(st, "cfg_clear", CFG_ERROR_NULL_PTR);
	return cfg_free_memory(st);
}

cfg_status_t cfg_free(cfg_t *st)
{
	cfg_status_t ret;

	CFG_CHECK_ST_RETURN(st, "cfg_free", CFG_ERROR_NULL_PTR);

	if (st->init != CFG_TRUE)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_INIT);
	if (st->nentries || st->nsections) {
		ret = cfg_free_memory(st);
		if (ret != CFG_STATUS_OK)
			return ret;
	}
	memset((void *)st, 0, sizeof(st));
	st->init = CFG_FALSE; /* not needed if CFG_FALSE is zero */

	free(st);
	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

static cfg_status_t cfg_parse_buffer_keys(cfg_t *st, cfg_char *buf, cfg_uint32 sz)
{
	cfg_uint32 section_hash = CFG_ROOT_SECTION_HASH;
	cfg_uint32 section_idx = 0;
	cfg_uint32 key_idx = 0;
	cfg_entry_t *entry;
	cfg_char *p, *end;

	for (p = buf; p < buf + sz; p++) {
		/* store current section hash */
		if (*p == st->separator_section) {
			p++;
			end = p;
			while (*end != st->separator_section)
				end++;
			*end = '\0';
			st->section[section_idx] = cfg_strdup(p);
			section_idx++;
			section_hash = cfg_hash_get(p);
			*end = st->separator_section;
			p = end;
			/* if next character is not a section start skip */
			if (*(p + 1) != st->separator_section)
				p++;
		}

		/* nentries is reached skip */
		if (key_idx == st->nentries)
			continue;

		/* a new entry */
		entry = &st->entry[key_idx];
		entry->section_hash = section_hash;
		key_idx++;

		/* parse key */
		end = p;
		end++;
		while (*end != st->separator_key_value)
			end++;
		*end = '\0';

		entry->key = cfg_strdup(p);
		*end = st->separator_key_value;
		end++;
		p = end;

		entry->key_hash = cfg_hash_get(entry->key);

		/* parse value */
		while (*end != st->separator_key_value)
			end++;
		*end = '\0';
		entry->value = cfg_strdup(p);
		*end = st->separator_key_value;
		p = end;
	}

	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_status_t cfg_parse_buffer(cfg_t *st, cfg_char *buf, cfg_uint32 sz, cfg_bool copy)
{
	cfg_status_t ret;
	cfg_char *newbuf;
	cfg_uint32 keys, sections;

	CFG_CHECK_ST_RETURN(st, "cfg_parse_buffer", CFG_ERROR_NULL_PTR);

	if (st->init != CFG_TRUE)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_INIT);

	/* set buffer */
	if (copy) {
		newbuf = (cfg_char *)malloc(sz + 1);
		if (!newbuf)
			CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
		memcpy(newbuf, buf, sz);
		newbuf[sz] = '\0';
	} else {
		newbuf = buf;
	}

	/* clear old keys */
	ret = cfg_free_memory(st);
	if (ret != CFG_STATUS_OK)
		CFG_SET_RETURN_STATUS(st, ret);
	ret = cfg_cache_size_set(st, st->cache_size);
	if (ret != CFG_STATUS_OK)
		CFG_SET_RETURN_STATUS(st, ret);
	cfg_cache_clear(st);
	if (ret != CFG_STATUS_OK)
		CFG_SET_RETURN_STATUS(st, ret);

	cfg_unescape(st, newbuf, sz, &keys, &sections);

	/* allocate memory for the list */
	if (keys) {
		st->entry = (cfg_entry_t *)malloc(keys * sizeof(cfg_entry_t));
		if (!st->entry)
			CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
	}
	if (sections) {
		st->section = (cfg_char **)malloc(sections * sizeof(cfg_char *));
		if (!st->section)
			CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
	}

	st->nentries = keys;
	st->nsections = sections;
	ret = cfg_parse_buffer_keys(st, newbuf, sz);
	if (copy)
		free(newbuf);
	CFG_SET_RETURN_STATUS(st, ret);
}

cfg_status_t cfg_parse_file_ptr(cfg_t *st, FILE *f, cfg_bool close)
{
	cfg_char *buf;
	cfg_uint32 sz = 0;
	cfg_status_t ret;

	CFG_CHECK_ST_RETURN(st, "cfg_parse_file_ptr", CFG_ERROR_NULL_PTR);
	if (!f)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_FILE);
	if (st->init != CFG_TRUE)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_INIT);

	/* get file size */
	while (fgetc(f) != EOF)
		sz++;
	rewind(f);

	buf = (cfg_char *)malloc(sz);
	if (!buf) {
		if (close)
			fclose(f);
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
	}
	if (fread(buf, 1, sz, f) != sz) {
		if (close)
			fclose(f);
		free(buf);
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_FREAD);
	}
	if (close)
		fclose(f);

	ret = cfg_parse_buffer(st, buf, sz, CFG_FALSE);
	free(buf);
	return ret;
}

cfg_status_t cfg_parse_file(cfg_t *st, cfg_char *filename)
{
	FILE *f;
	CFG_CHECK_ST_RETURN(st, "cfg_parse_file", CFG_ERROR_NULL_PTR);
	if (st->init != CFG_TRUE)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_INIT);
	/* read file */
	f = fopen(filename, "r");
	return cfg_parse_file_ptr(st, f, CFG_TRUE);
}

/* characters to escape: '[', ']', '=', '"', '\', '\n' */
static cfg_char *cfg_escape(cfg_char *str, cfg_uint32 *len)
{
	cfg_uint32 n;
	cfg_char *src = str, *dest, *buf;

	*len = 0;
	if (!src)
		return "";
	n = strlen(src);
	if (!n)
		return "";
	buf = (cfg_char *)malloc(n * 2 + 1);
	dest = buf;

	while (*src) {
		switch (*src) {
		case '[':
		case ']':
		case '=':
		case '"':
			*dest = '\\';
			dest++;
			*dest = *src;
			break;
		case '\n':
			*dest = '\\';
			dest++;
			*dest = 'n';
			break;
		default:
			*dest = *src;
		}
		dest++;
		src++;
	}
	*dest = '\0';
	*len = dest - buf;
	return buf;
}

static cfg_char* cfg_entry_string(cfg_entry_t *entry, cfg_uint32 *len)
{
	cfg_char *buf, *key, *value;
	cfg_uint32 key_len, value_len;

	key = cfg_escape(entry->key, &key_len);
	value = cfg_escape(entry->value, &value_len);

	*len = key_len + value_len + 6;  /* 4x '"', '=', '\n' */
	buf = (cfg_char *)malloc(*len + 1);
	if (!buf)
		return NULL;

	buf[0] = '\0';
	strcat(buf, "\"");
	strcat(buf, key);
	strcat(buf, "\"=\"");
	strcat(buf, value);
	strcat(buf, "\"\n");

	if (key_len)
		free(key);
	if (value_len)
		free(value);
	return buf;
}

cfg_status_t cfg_write_buffer(cfg_t *st, cfg_char **out, cfg_uint32 *len)
{
	static const cfg_char *fname = "[cfg2] cfg_write_buffer():";
	cfg_uint32 i, j, nsec, n;
	cfg_char *ptr, *str;
	cfg_char **sec_buf;
	cfg_uint32 *sec_hash;
	cfg_uint32 *sec_len;
	cfg_entry_t *entry;

	CFG_CHECK_ST_RETURN(st, "cfg_write_buffer", CFG_ERROR_NULL_PTR);
	if (st->init != CFG_TRUE)
		return CFG_ERROR_INIT;
	if (!out || !len)
		return CFG_ERROR_NULL_PTR;

	nsec = st->nsections + 1;
	sec_hash = (cfg_uint32 *)malloc(nsec * sizeof(cfg_uint32));
	sec_len = (cfg_uint32 *)malloc(nsec * sizeof(cfg_uint32));
	sec_buf = (cfg_char **)malloc(nsec * sizeof(cfg_char *));
	sec_buf[0] = (cfg_char *)malloc(1);

	if (!sec_hash || !sec_len || !sec_buf || !sec_buf[0])
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);

	sec_buf[0][0] = '\0';
	sec_hash[0] = CFG_ROOT_SECTION_HASH;
	sec_len[0] = 0;

	if (st->verbose > 0)
		fprintf(stderr, "%s allocating sections...\n", fname);

	for (i = 0; i < st->nsections; i++) {
		sec_hash[i + 1] = cfg_hash_get(st->section[i]);

		str = cfg_escape(st->section[i], &n);
		n += 3; /* [, ], \n */
		sec_len[i + 1] = n;
		sec_buf[i + 1] = (cfg_char *)malloc(n + 1);
		if (!sec_buf[i + 1])
			CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
		ptr = sec_buf[i + 1];
		ptr[0] = '\0';
		strcat(ptr, "[");
		strcat(ptr, str);
		strcat(ptr, "]\n");
		free(str);
	}

	if (st->verbose > 0)
		fprintf(stderr, "%s allocating entries per section...\n", fname);

	for (i = 0; i < st->nentries; i++) {
		entry = &st->entry[i];
		if (!entry)
			CFG_SET_RETURN_STATUS(st, CFG_ERROR_NULL_PTR);
		for (j = 0; j < nsec; j++) {
			if (sec_hash[j] == entry->section_hash)
				break;
		}
		str = cfg_entry_string(entry, &n);
		if (!str)
			CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
		sec_buf[j] = realloc(sec_buf[j], sec_len[j] + n + 1);
		if (!sec_buf[j])
			CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
		sec_len[j] += n;
		strcat(sec_buf[j], str);
		free(str);
	}

	if (st->verbose > 0)
		fprintf(stderr, "%s combining sections...\n", fname);

	*out = NULL;
	*len = 0;
	for (i = 0; i < nsec; i++)
		*len += sec_len[i] + 1;
	*out = (cfg_char *)malloc(*len + 1);
	if (!*out)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
	*out[0] = '\0';
	for (i = 0; i < nsec; i++) {
		strcat(*out, sec_buf[i]);
		strcat(*out, "\n");
		free(sec_buf[i]);
	}
	free(sec_buf);
	free(sec_len);
	free(sec_hash);

	if (st->verbose > 0)
		fprintf(stderr, "%s done!\n", fname);

	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_status_t cfg_write_file_ptr(cfg_t *st, FILE *f, cfg_bool close)
{
	cfg_char *buf;
	cfg_uint32 sz = 0, sz_write = 0;
	cfg_status_t ret;

	CFG_CHECK_ST_RETURN(st, "cfg_write_file_ptr", CFG_ERROR_NULL_PTR);
	if (st->init != CFG_TRUE)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_INIT);
	if (!f)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_FILE);

	ret = cfg_write_buffer(st, &buf, &sz);
	if (ret != CFG_STATUS_OK || !buf || !sz) {
		if (close)
			fclose(f);
		return ret;
	}

	rewind(f);
	sz_write = fwrite(buf, 1, sz, f);
	free(buf);

	if (sz_write != sz)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_FWRITE);
	if (close)
		fclose(f);
	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_status_t cfg_write_file(cfg_t *st, cfg_char *filename)
{
	FILE *f;
	CFG_CHECK_ST_RETURN(st, "cfg_write_file", CFG_ERROR_NULL_PTR);
	if (st->init != CFG_TRUE)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_INIT);
	/* read file */
	f = fopen(filename, "w");
	return cfg_write_file_ptr(st, f, CFG_TRUE);
}
