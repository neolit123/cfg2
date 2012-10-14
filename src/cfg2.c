/*
 * cfg2 0.1
 * a simplistic configuration parser for INI like syntax in ANSI C
 *
 * author: lubomir i. ivanov (neolit123 at gmail)
 * this code is released in the public domain without warranty of any kind.
 * providing credit to the original author is recommended but not mandatory. 
 *
 * cfg2.c:
 *	this file holds the library definitions
 */
#include "cfg2.h"

void cfg_cache_clear(cfg_t *st)
{
	if (st->cache_size && st->init == CFG_TRUE) {
		memset((void *)st->cache_keys_hash, 0, st->cache_size * sizeof(cfg_uint32));
		memset((void *)st->cache_keys_index, 0, st->cache_size * sizeof(cfg_int));
	}
}

cfg_error_t cfg_cache_size_set(cfg_t *st, cfg_int size)
{
	if (st->init != CFG_TRUE)
		return CFG_ERROR_INIT;
	if (size < 0)
		return CFG_ERROR_CRITICAL;
	st->cache_size = size;
	if (size == 0)
		return CFG_ERROR_OK;
	st->cache_keys_hash = (cfg_uint32 *)realloc(st->cache_keys_hash, size * sizeof(cfg_uint32));
	st->cache_keys_index = (cfg_int *)realloc(st->cache_keys_index, size * sizeof(cfg_int));
	if (!st->cache_keys_index || !st->cache_keys_hash)
		return CFG_ERROR_ALLOC;
	cfg_cache_clear(st);
	return CFG_ERROR_OK;
}

cfg_error_t cfg_init(cfg_t *st)
{
	st->keys_hash = NULL;
	st->values_hash = NULL;
	st->keys = NULL;
	st->values = NULL;
	st->buf = NULL;
	st->file = NULL;
	st->nkeys = 0;
	st->buf_size = 0;
	st->cache_keys_index = NULL;
	st->cache_keys_hash = NULL;
	st->init = CFG_TRUE;
	st->cache_size = CFG_CACHE_SIZE;
	return CFG_ERROR_OK;
}

/* fast fnv-32 hash */
static cfg_uint32 cfg_hash_get(cfg_char *str)
{
	cfg_uint32 hash = 0x811c9dc5;
	if (str)
		while(*str) {
			hash += (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24);
			hash ^= *str++;
		}
	return hash;
}

#define cfg_escape_char_trim(c, p, end) \
	*p = c; \
	memmove(p + 1, p + 2, end - (p + 1)); \
	*((end)--) = '\0'

#define cfg_multi_line_trim(p, end) \
	memmove(p, p + 2, end - (p + 2)); \
	end -= 2; \
	*(end) = '\0'

/* escape all special characters (like \n) in a string */
static cfg_char *cfg_value_escape(cfg_char *str) {
	cfg_char *p = str, *end = str + strlen(str);
	cfg_char next;

	while (*p) {
		if (*p == '\\') {
			next = *(p + 1);
			switch (next) {
			case '\n': /* multi-line value: remove the \\ and \n characters */
				cfg_multi_line_trim(p, end);
				break;
			case 'n':
				cfg_escape_char_trim('\n', p, end);
				break;
			case 't':
				cfg_escape_char_trim('\t', p, end);
				break;
			case 'r':
				cfg_escape_char_trim('\r', p, end);
				break;
			case 'v':
				cfg_escape_char_trim('\v', p, end);
				break;
			case 'f':
				cfg_escape_char_trim('\f', p, end);
				break;
			case 'b':
				cfg_escape_char_trim('\b', p, end);
				break;
			}
		}
		p++;
	}
	return str;
}

cfg_char* cfg_key_nth(cfg_t *st, cfg_int n)
{
	if (n < 0 || n > st->nkeys - 1 || !st->nkeys)
		return NULL;
	return st->keys[n];
}

cfg_char* cfg_value_nth(cfg_t *st, cfg_int n)
{
	if (n < 0 || n > st->nkeys - 1 || !st->nkeys)
		return NULL;
	return st->values[n];
}

cfg_int cfg_key_get_index(cfg_t *st, cfg_char *key)
{
	cfg_int i = 0;
	cfg_uint32 hash;

	if (!key || !st->nkeys)
		return -1;
	hash = cfg_hash_get(key);
	while (i < st->nkeys) {
		if (hash == st->keys_hash[i])
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
		if (hash == st->values_hash[i])
			return st->keys[i];
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
				return st->values[st->cache_keys_index[i]];
			i++;
		}
	}

	/* check for value in main list */
	i = 0;
	while (i < st->nkeys) {
		if (hash == st->keys_hash[i]) {
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
			return st->values[i];
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
	cfg_int len = strlen(value);
	cfg_uint32 hash_key;

	if (!value || !key || st->nkeys == 0)
		return CFG_ERROR_CRITICAL;
	hash_key = cfg_hash_get(key);
	value = cfg_value_escape(value);
	while (i < st->nkeys) {
		if (hash_key == st->keys_hash[i]) {
			st->values_hash[i] = cfg_hash_get(value);
			if (st->values[i])
				free(st->values[i]);
			st->values[i] = (cfg_char *)malloc((len + 1) * sizeof(cfg_char));
			if (!st->values[i])
				return CFG_ERROR_ALLOC;
			strncpy(st->values[i], value, len);
			st->values[i][len] = '\0';
			return CFG_ERROR_OK;
		}
		i++;
	}
	return CFG_ERROR_KEY_NOT_FOUND;
}

static cfg_error_t cfg_free_memory(cfg_t *st)
{
	cfg_int i = 0;

	if ((!st->keys || !st->values || !st->keys_hash || !st->values_hash) && st->nkeys)
		return CFG_ERROR_CRITICAL;
	if (!st->nkeys) /* nothing to do here */
		return CFG_ERROR_OK;

	while (i < st->nkeys) {
		if (!st->keys[i]) {
			return CFG_ERROR_NULL_KEY;
		}	else {
			free(st->keys[i]);
			st->keys[i] = NULL;
		}
		if (st->values[i]) {
			free(st->values[i]);
			st->values[i] = NULL;
		}
		i++;
	}
	free(st->keys);
	st->keys = NULL;
	free(st->values);
	st->values = NULL;

	free(st->keys_hash);
	st->keys_hash = NULL;
	free(st->values_hash);
	st->values_hash = NULL;

	free(st->cache_keys_index);
	st->cache_keys_index = NULL;
	free(st->cache_keys_hash);
	st->cache_keys_hash = NULL;
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
		st->buf_size = 0;

		if (st->file)
			fclose(st->file);
		st->file = NULL;
	}
	memset((void *)st, 0, sizeof(st));
	st->init = CFG_FALSE; /* not needed if CFG_FALSE is zero */
	return CFG_ERROR_OK;
}

/* a couple of helper macros */
#define cfg_check_endline_multiline(bp, buf) \
	*bp == '\n' && (bp > buf && *(bp - 1) != '\\')

#define cfg_check_line_equalsign(bp, ec, ls) \
	*bp = '\0'; \
	ec = strchr(ls, '='); \
	*bp = '\n'; \
	if (bp - ls == 0 || *ls == '#' || !ec) { \
		ls = bp + 1; \
		bp++; \
		continue; \
	}

cfg_error_t cfg_parse_buffer(cfg_t *st, cfg_char *buf, cfg_int sz)
{
	cfg_int n, ret, nkeys;
	cfg_char *bp, *bend, *ls, *ec;

	if (st->init != CFG_TRUE)
		return CFG_ERROR_INIT;

	/* set buffer */
	st->buf = buf;
	st->buf_size = sz;

	/* clear old keys */
	ret = cfg_free_memory(st);
	if (ret > 0)
		return ret;
	ret = cfg_cache_size_set(st, st->cache_size);
	if (ret > 0)
		return ret;

	/* find the number of keys by going trough the entire buffer initially.
	 * we use this method instead of a growing list, since realloc can end up
	 * being slow. even if our estimate is off a little realloc will have to copy
	 * all pointer values for the buffer to grow */
	ls = bp = st->buf;
	bend = bp + sz;
	n = nkeys = 0;
	while (bp < bend) {
		/* check if this is a multi-line definition */
		if (cfg_check_endline_multiline(bp, st->buf)) {
			cfg_check_line_equalsign(bp, ec, ls);
			nkeys++;
			ls = bp + 1;
		}
		bp++;
	}

	/* allocate memory for the list */
	st->keys = (cfg_char **)malloc(nkeys * sizeof(cfg_char *));
	st->values = (cfg_char **)malloc(nkeys * sizeof(cfg_char *));
	st->keys_hash = (cfg_uint32 *)malloc(nkeys * sizeof(cfg_uint32));
	st->values_hash = (cfg_uint32 *)malloc(nkeys * sizeof(cfg_uint32));
	if (!st->keys || !st->values || !st->keys_hash || !st->values_hash)
		return CFG_ERROR_ALLOC;

	/* fill keys and values */
	ls = bp = st->buf;
	bend = bp + sz;
	n = 0;
	while (bp < bend) {
		/* check if this is a multi-line definition */
		if (cfg_check_endline_multiline(bp, st->buf)) {
			cfg_check_line_equalsign(bp, ec, ls);
			/* fill key */
			sz = ec - ls + 1;
			st->keys[n] = (cfg_char *)malloc(sz * sizeof(cfg_char));
			if (!st->keys[n])
				return CFG_ERROR_ALLOC;
			strncpy(st->keys[n], ls, sz);
			st->keys[n][sz - 1] = '\0';
			st->keys_hash[n] = cfg_hash_get(st->keys[n]);

			/* fill key value */
			sz = bp - ec;
			if (sz - 1 == 0) { /* empty value */
				st->values[n] = NULL;
				st->values_hash[n] = cfg_hash_get(NULL);
				ls = bp + 1;
				bp++;
				n++;
				continue;
			}
			st->values[n] = (cfg_char *)malloc(sz * sizeof(cfg_char));
			if (!st->values[n])
				return CFG_ERROR_ALLOC;
			strncpy(st->values[n], ec + 1, sz);
			st->values[n][sz - 1] = '\0';
			st->values_hash[n] = cfg_hash_get(cfg_value_escape(st->values[n]));
			ls = bp + 1;
			n++;
		}
		bp++;
	}
	st->nkeys = nkeys;
	return CFG_ERROR_OK;
}

cfg_error_t cfg_parse_file(cfg_t *st, cfg_char *filename)
{
	FILE *f;
	cfg_char *buf;
	cfg_int sz = 0, ret;

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

	buf = (cfg_char *)malloc(sz * sizeof(cfg_char));
	if (!buf) {
		fclose(f);
		st->file = NULL;
		return CFG_ERROR_ALLOC;
	}
	if (fread(buf, sizeof(cfg_char), sz, f) != (cfg_uint32)sz) {
		fclose(f);
		return CFG_ERROR_FREAD;
	}
	fclose(f);
	st->file = NULL;

	ret = cfg_parse_buffer(st, buf, sz);
	free(st->buf);
	st->buf = NULL;
	return ret;
}
