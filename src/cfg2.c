/*
 * cfg2
 * a simplistic configuration parser for INI like syntax in ANSI C
 *
 * author: lubomir i. ivanov (neolit123 at gmail)
 * this code is released in the public domain without warranty of any kind.
 * providing credit to the original author is recommended but not mandatory.
 *
 * cfg2.c:
 *	this file holds the library definitions
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cfg2.h"

static const char key_value_separator = 0x1;

void cfg_cache_clear(cfg_t *st)
{
	/* clearing the cache buffers technically sets them to the first index (0) */
	if (st->cache_size && st->init == CFG_TRUE) {
		memset((void *)st->cache_keys_hash, 0, st->cache_size * sizeof(cfg_uint32));
		memset((void *)st->cache_keys_index, 0, st->cache_size * sizeof(cfg_int));
	}
}

cfg_error_t cfg_cache_size_set(cfg_t *st, cfg_int size)
{
	int diff;
	if (st->init != CFG_TRUE)
		return CFG_ERROR_INIT;
	if (size < 0)
		return CFG_ERROR_CRITICAL;
	/* check if we are setting the buffers to zero length */
	if (size == 0) {
		if (st->cache_keys_hash)
			free(st->cache_keys_hash);
		st->cache_keys_hash = NULL;
		if (st->cache_keys_index)
			free(st->cache_keys_index);
		st->cache_keys_index = NULL;
	} else {
		st->cache_keys_hash = (cfg_uint32 *)realloc(st->cache_keys_hash, size * sizeof(cfg_uint32));
		st->cache_keys_index = (cfg_int *)realloc(st->cache_keys_index, size * sizeof(cfg_int));
		if (!st->cache_keys_index || !st->cache_keys_hash)
			return CFG_ERROR_ALLOC;
		/* if the new buffers are larger, lets fill the extra indexes with zeroes */
		if (size > st->cache_size) {
			diff = size - st->cache_size;
			memset((void *)&st->cache_keys_hash[st->cache_size], 0, diff * sizeof(cfg_uint32));
			memset((void *)&st->cache_keys_index[st->cache_size], 0, diff * sizeof(cfg_int));
		}
	}
	st->cache_size = size;
	return CFG_ERROR_OK;
}

cfg_error_t cfg_init(cfg_t *st, cfg_int cache_size)
{
	st->entry = NULL;
	st->buf = NULL;
	st->file = NULL;
	st->nkeys = 0;
	st->buf_size = 0;
	st->cache_keys_index = NULL;
	st->cache_keys_hash = NULL;
	st->init = CFG_TRUE;
	if (cache_size < 0)
		st->cache_size = CFG_CACHE_SIZE;
	else
		st->cache_size = cache_size;
	return CFG_ERROR_OK;
}

/* fast fnv-32 hash */
static cfg_uint32 cfg_hash_get(cfg_char *str)
{
	cfg_uint32 hash = 0x811c9dc5;
	if (!str)
		return hash;
	while (*str) {
		hash *= hash;
		hash ^= *str++;
	}
	return hash;
}

#define cfg_escape_special_char(char1, char2) \
	case char1: \
		*dst = char2; \
		dst++; \
		continue

/* escape all special characters (like \n) in a string */
static cfg_uint32 cfg_escape(cfg_char *str)
{
	char *src, *dst;
	int escape = 0;
	cfg_uint32 keys = 0;

	for (src = dst = str; *src != '\0'; src++) {
		if (*src == '#' && ((src > str && *(src - 1) == '\n') || src == str)) {
			while (*src != '\n')
				src++;
			continue;
		}
		*dst = *src;
		if (escape) {
			escape = 0;
			switch (*dst) {
			case '\n':
				continue;
			cfg_escape_special_char('t', '\t');
			cfg_escape_special_char('r', '\r');
			cfg_escape_special_char('v', '\v');
			cfg_escape_special_char('b', '\b');
			}
		} else {
			switch (*dst) {
			case '=':
				keys++;
			case '\n':
				*dst = key_value_separator;
				dst++;
				continue;
			}
		}
		escape = 0;
		if (*dst == '\\') {
			escape = 1;
			continue;
		}
		dst++;
	}
	*dst = '\0';
	return keys;
}

cfg_entry_t *cfg_entry_nth(cfg_t *st, cfg_int n)
{
	if (n < 0 || n > st->nkeys - 1 || !st->nkeys)
		return NULL;
	return &(st->entry[n]);
}

cfg_char* cfg_key_nth(cfg_t *st, cfg_int n)
{
	if (n < 0 || n > st->nkeys - 1 || !st->nkeys)
		return NULL;
	return st->entry[n].key;
}

cfg_char* cfg_value_nth(cfg_t *st, cfg_int n)
{
	if (n < 0 || n > st->nkeys - 1 || !st->nkeys)
		return NULL;
	return st->entry[n].value;
}

cfg_int cfg_key_get_index(cfg_t *st, cfg_char *key)
{
	cfg_int i = 0;
	cfg_uint32 hash;

	if (!key || !st->nkeys)
		return -1;
	hash = cfg_hash_get(key);
	while (i < st->nkeys) {
		if (hash == st->entry[i].key_hash)
			return i;
		i++;
	}
	return -1;
}

cfg_char* cfg_key_get(cfg_t *st, cfg_char *value)
{
	cfg_int i = 0;
	cfg_uint32 hash;

	if (!value || !st->nkeys)
		return NULL;
	hash = cfg_hash_get(value);
	while (i < st->nkeys) {
		if (hash == st->entry[i].value_hash)
			return st->entry[i].key;
		i++;
	}
	return NULL;
}

cfg_char* cfg_value_get(cfg_t *st, cfg_char *key)
{
	cfg_int i;
	cfg_uint32 hash;

	if (!key || !st->nkeys)
		return NULL;
	hash = cfg_hash_get(key);

	/* check for value in cache first */
	if (st->cache_size > 0) {
		i = 0;
		while (i < st->cache_size) {
			if (hash == st->cache_keys_hash[i])
				return st->entry[st->cache_keys_index[i]].value;
			i++;
		}
	}

	/* check for value in main list */
	i = 0;
	while (i < st->nkeys) {
		if (hash == st->entry[i].key_hash) {
			if (st->cache_size > 0) {
				/* lets add to the cache */
				if (st->cache_size > 1) {
					memmove((void *)(st->cache_keys_hash + 1), (void *)st->cache_keys_hash,
					        (st->cache_size - 1) * sizeof(cfg_uint32));
					memmove((void *)(st->cache_keys_index + 1), (void *)st->cache_keys_index,
					        (st->cache_size - 1) * sizeof(cfg_int));
				}
				st->cache_keys_hash[0] = hash;
				st->cache_keys_index[0] = i;
			}
			return st->entry[i].value;
		}
		i++;
	}
	return NULL;
}

cfg_long cfg_value_get_long(cfg_t *st, cfg_char *key, cfg_int base)
{
	cfg_char *str = cfg_value_get(st, key);
	if (str)
		return strtol(str, NULL, base);
	return 0;
}

cfg_ulong cfg_value_get_ulong(cfg_t *st, cfg_char *key, cfg_int base)
{
	cfg_char *str = cfg_value_get(st, key);
	if (str)
		return strtoul(str, NULL, base);
	return 0;
}

/* msvcr does not support strtof so we only use strtod for doubles */
cfg_double cfg_value_get_double(cfg_t *st, cfg_char *key)
{
	cfg_char *str = cfg_value_get(st, key);
	if (str)
		return strtod(str, NULL);
	return 0.0;
}

cfg_error_t cfg_value_set(cfg_t *st, cfg_char *key, cfg_char *value)
{
	cfg_int i = 0;
	cfg_uint32 hash_key;
	cfg_entry_t *entry;

	if (!value || !key || st->nkeys == 0)
		return CFG_ERROR_CRITICAL;

	hash_key = cfg_hash_get(key);

	while (i < st->nkeys) {
		entry = &st->entry[i];
		if (hash_key == entry->key_hash) {
			entry->value_hash = cfg_hash_get(value);
			if (entry->value)
				free(entry->value);
			entry->value = strdup(value);
			if (!entry->value)
				return CFG_ERROR_ALLOC;
			cfg_escape(entry->value);
			return CFG_ERROR_OK;
		}
		i++;
	}
	return CFG_ERROR_KEY_NOT_FOUND;
}

static cfg_error_t cfg_free_memory(cfg_t *st)
{
	cfg_int i = 0;

	if (!st->entry && st->nkeys)
		return CFG_ERROR_CRITICAL;
	if (!st->nkeys) /* nothing to do here */
		return CFG_ERROR_OK;

	while (i < st->nkeys) {
		if (!st->entry[i].key) {
			return CFG_ERROR_NULL_KEY;
		} else {
			free(st->entry[i].key);
			st->entry[i].key = NULL;
		}
		if (st->entry[i].value) {
			free(st->entry[i].value);
			st->entry[i].value = NULL;
		}
		i++;
	}
	free(st->entry);
	st->entry = NULL;
	if (st->cache_keys_index) {
		free(st->cache_keys_index);
		st->cache_keys_index = NULL;
	}
	if (st->cache_keys_hash) {
		free(st->cache_keys_hash);
		st->cache_keys_hash = NULL;
	}
	st->nkeys = 0;
	return CFG_ERROR_OK;
}

cfg_error_t cfg_free(cfg_t *st)
{
	cfg_int ret;

	if (st->init != CFG_TRUE)
		return CFG_ERROR_INIT;
	if (st->nkeys) {
		ret = cfg_free_memory(st);
		if (ret > 0)
			return ret;

		st->buf = NULL;

		if (st->file)
			fclose(st->file);
		st->file = NULL;
	}
	memset((void *)st, 0, sizeof(st));
	st->init = CFG_FALSE; /* not needed if CFG_FALSE is zero */
	return CFG_ERROR_OK;
}

static cfg_error_t cfg_parse_buffer_keys(cfg_t *st)
{
	cfg_uint32 nkeys = st->nkeys;
	cfg_char *buf = st->buf;

	cfg_uint32 i;
	cfg_entry_t *entry;

	cfg_char *p, *end;
	p = buf;

	for (i = 0; i < nkeys; i++) {
		entry = &st->entry[i];
		entry->index = i;

		end = p;
		while (*end != key_value_separator)
			end++;
		*end = '\0';
		entry->key = strdup(p);
		*end = key_value_separator;
		end++;
		p = end;

		entry->key_hash = cfg_hash_get(entry->key);

		while (*end != key_value_separator)
			end++;
		*end = '\0';
		entry->value = strdup(p);
		*end = key_value_separator;
		end++;
		p = end;

		entry->value_hash = cfg_hash_get(entry->value);
	}

	return CFG_ERROR_OK;
}

cfg_error_t cfg_parse_buffer(cfg_t *st, cfg_char *buf, cfg_uint32 sz)
{
	cfg_int ret;
	cfg_uint32 nkeys;
	(void)sz;

	if (st->init != CFG_TRUE)
		return CFG_ERROR_INIT;

	/* set buffer */
	st->buf = buf;

	/* clear old keys */
	ret = cfg_free_memory(st);
	if (ret > 0)
		return ret;
	ret = cfg_cache_size_set(st, st->cache_size);
	if (ret > 0)
		return ret;
	cfg_cache_clear(st);
	if (ret > 0)
		return ret;

	nkeys = cfg_escape(buf);

	/* allocate memory for the list */
	st->entry = (cfg_entry_t *)malloc(nkeys * sizeof(cfg_entry_t));
	if (!st->entry)
		return CFG_ERROR_ALLOC;

	st->nkeys = nkeys;
	return cfg_parse_buffer_keys(st);
}

cfg_error_t cfg_parse_file(cfg_t *st, cfg_char *filename)
{
	FILE *f;
	cfg_char *buf;
	cfg_uint32 sz = 0;
	cfg_int ret;

	if (st->init != CFG_TRUE)
		return CFG_ERROR_INIT;

	/* read file */
	f = fopen(filename, "r");
	if (!f)
		return CFG_ERROR_FOPEN;
	st->file = f;

	/* get file size */
	while (fgetc(f) != EOF)
		sz++;
	rewind(f);

	buf = (cfg_char *)malloc((sz + 1) * sizeof(cfg_char));
	if (!buf) {
		fclose(f);
		st->file = NULL;
		return CFG_ERROR_ALLOC;
	}
	if (fread(buf, sizeof(cfg_char), sz, f) != sz) {
		fclose(f);
		return CFG_ERROR_FREAD;
	}
	fclose(f);
	st->file = NULL;

	buf[sz] = '\0';
	ret = cfg_parse_buffer(st, buf, 0);
	free(st->buf);
	st->buf = NULL;
	return ret;
}
