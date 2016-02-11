/*
 * cfg2
 * a simplistic configuration parser for INI like syntax in C
 *
 * author: lubomir i. ivanov (neolit123 at gmail)
 * this code is released in the public domain without warranty of any kind.
 * providing credit to the original author is recommended but not mandatory.
 *
 * core.c:
 *	initialization, i/o and core related functions
 */

#include "defines.h"

static void cfg_init(cfg_t *st)
{
	st->separator_key_value = CFG_SEPARATOR_KEY_VALUE;
	st->separator_section = CFG_SEPARATOR_SECTION;
	st->comment_char1 = CFG_COMMENT_CHAR1;
	st->comment_char2 = CFG_COMMENT_CHAR2;

	st->verbose = 0;
	st->status = CFG_STATUS_OK;

	st->section = NULL;
	st->nsections = 0;

	st->cache = NULL;
	st->cache_size = CFG_CACHE_SIZE;
}

cfg_t *cfg_alloc(void)
{
	cfg_t *st = (cfg_t *)calloc(1, sizeof(cfg_t));
	if (!st) {
		fprintf(stderr, "[cfg2] cfg_alloc(): cannot calloc() a cfg_t object!\n");
		return NULL;
	}
	cfg_init(st);
	cfg_cache_size_set(st, st->cache_size);
	return st;
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

static cfg_status_t cfg_memory_free(cfg_t *st)
{
	cfg_section_t *section;
	cfg_entry_t *entry;
	cfg_uint32 i, j;

	for (i = 0; i < st->nsections; i++) {
		section = &st->section[i];
		for (j = 0; j < section->nentries; j++) {
			entry = &section->entry[j];
			free(entry->key);
			free(entry->value);
		}
		free(section->name);
		free(section->entry);
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
	cfg_status_t ret;
	CFG_CHECK_ST_RETURN(st, "cfg_clear", CFG_ERROR_NULL_PTR);
	ret = cfg_memory_free(st);
	if (ret != CFG_STATUS_OK)
		return ret;
	cfg_cache_size_set(st, st->cache_size); /* recreate the cache */
	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_status_t cfg_free(cfg_t *st)
{
	cfg_status_t ret;

	CFG_CHECK_ST_RETURN(st, "cfg_free", CFG_ERROR_NULL_PTR);
	ret = cfg_memory_free(st);
	if (ret != CFG_STATUS_OK)
		return ret;
	free(st);
	return CFG_STATUS_OK;
}

static void cfg_raw_buffer_parse(cfg_t *st, cfg_char *buf, cfg_uint32 sz, cfg_uint32 sections, cfg_uint32 **entries)
{
	cfg_uint32 idx_section = 0;
	cfg_uint32 idx_entry = 0;
	cfg_entry_t *entry;
	cfg_char *p, *end;
	cfg_uint32 i, *entry_ptr;
	cfg_section_t *section;

	/* allocate section and entry buffers */
	entry_ptr = *entries;
	st->nsections = sections;
	st->section = (cfg_section_t *)malloc(sections * sizeof(cfg_section_t));
	for (i = 0; i < sections; i++) {
		section = &st->section[i];
		section->nentries = entry_ptr[i];
		section->entry = !section->nentries ? NULL : (cfg_entry_t *)malloc(section->nentries * sizeof(cfg_entry_t));
	}

	/* prepare the root section */
	section = &st->section[0];
	section->name = CFG_ROOT_SECTION;
	section->hash = CFG_ROOT_SECTION_HASH;

	for (p = buf; p < buf + sz; p++) {
		/* store a new section */
		if (*p == st->separator_section) {
			p++;
			end = p;
			while (*end != st->separator_section)
				end++;
			*end = '\0';
			idx_entry = 0;
			idx_section++;
			section = &st->section[idx_section];
			section->name = cfg_strdup(p);
			section->hash = cfg_hash_get(p);
			*end = st->separator_section;
			p = end;
			/* if next character is not a section start skip */
			if (*(p + 1) != st->separator_section)
				p++;
		}

		/* nentries is reached skip */
		if (idx_entry == section->nentries)
			continue;

		/* a new entry */
		entry = &section->entry[idx_entry];
		entry->section = section;
		idx_entry++;

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
}

#define CFG_UNESCAPE_CHECK_QUOTE() \
	if (quote && st->verbose > 0) \
		fprintf(stderr, "%s: WARNING: quote not closed at line %d\n", fname, line); \
	quote = CFG_FALSE;

/* unescape all special characters (like \n) in a string and convert to a raw buffer */
static void cfg_raw_buffer_convert(cfg_t *st, cfg_char *buf, cfg_uint32 buf_sz, cfg_uint32 *sections, cfg_uint32 **entries)
{
	static const cfg_char *fname = "[cfg2] cfg_raw_buffer_convert()";
	cfg_uint32 line = 0, allocated, *entry_ptr, tmp_sz;
	cfg_char *src, *dest, last_char = 0;
	cfg_bool escape = CFG_FALSE;
	cfg_bool quote = CFG_FALSE;
	cfg_bool line_eq_sign = CFG_FALSE;
	cfg_bool multiline = CFG_FALSE;
	cfg_bool section_line = CFG_FALSE;

	/* prepare the root section */
	allocated = 1;
	*entries = (cfg_uint32 *)malloc(allocated * sizeof(cfg_uint32));
	entry_ptr = *entries;
	entry_ptr[0] = 0;
	*sections = 1;

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
				entry_ptr[*sections - 1]++;
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
				if ((*sections + 1) > allocated) {
					allocated <<= 1;
					tmp_sz = allocated * sizeof(cfg_uint32);
					*entries = (cfg_uint32 *)realloc(*entries, tmp_sz);
					if (!*entries) {
						fprintf(stderr, "%s: ERROR: cannot realloc() %d bytes\n", fname, tmp_sz);
						return;
					}
				}
				entry_ptr = *entries;
				entry_ptr[*sections] = 0;
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
	tmp_sz = *sections * sizeof(cfg_uint32);
	*entries = (cfg_uint32 *)realloc(*entries, tmp_sz); /* trim */
	if (!*entries) {
		fprintf(stderr, "%s: ERROR: cannot realloc() %d bytes\n", fname, tmp_sz);
		return;
	}
	if (st->verbose > 1)
		fprintf(stderr, "%s:\n%s\n", fname, buf);
}

cfg_status_t cfg_buffer_parse(cfg_t *st, cfg_char *buf, cfg_uint32 sz, cfg_bool copy)
{
	cfg_status_t ret;
	cfg_char *newbuf;
	cfg_uint32 sections, *entries = NULL;

	CFG_CHECK_ST_RETURN(st, "cfg_buffer_parse", CFG_ERROR_NULL_PTR);

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
	ret = cfg_memory_free(st);
	if (ret != CFG_STATUS_OK)
		CFG_SET_RETURN_STATUS(st, ret);
	ret = cfg_cache_size_set(st, st->cache_size);
	if (ret != CFG_STATUS_OK)
		CFG_SET_RETURN_STATUS(st, ret);
	cfg_cache_clear(st);
	if (ret != CFG_STATUS_OK)
		CFG_SET_RETURN_STATUS(st, ret);

	cfg_raw_buffer_convert(st, newbuf, sz, &sections, &entries);
	cfg_raw_buffer_parse(st, newbuf, sz, sections, &entries);
	free(entries);

	if (copy)
		free(newbuf);
	CFG_SET_RETURN_STATUS(st, ret);
}

cfg_status_t cfg_file_ptr_parse(cfg_t *st, FILE *f, cfg_bool close)
{
	cfg_char *buf;
	cfg_uint32 sz = 0;
	cfg_status_t ret;

	CFG_CHECK_ST_RETURN(st, "cfg_file_ptr_parse", CFG_ERROR_NULL_PTR);
	if (!f)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_FILE);

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

	ret = cfg_buffer_parse(st, buf, sz, CFG_FALSE);
	free(buf);
	return ret;
}

cfg_status_t cfg_file_parse(cfg_t *st, cfg_char *filename)
{
	FILE *f;
	CFG_CHECK_ST_RETURN(st, "cfg_file_parse", CFG_ERROR_NULL_PTR);
	/* read file */
	f = fopen(filename, "r");
	return cfg_file_ptr_parse(st, f, CFG_TRUE);
}

/* characters to escape: '[', ']', '=', '"', '\', '\n' */
static cfg_char *cfg_escape(cfg_char *str, cfg_uint32 *len)
{
	cfg_uint32 n;
	cfg_char *src = str, *dest, *buf;

	*len = 0;
	if (!src)
		return cfg_strdup("");
	n = strlen(src);
	if (!n)
		return cfg_strdup("");
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

	free(key);
	free(value);
	return buf;
}

cfg_status_t cfg_buffer_write(cfg_t *st, cfg_char **out, cfg_uint32 *len)
{
	static const cfg_char *fname = "[cfg2] cfg_buffer_write():";
	cfg_uint32 i, j, sz, n, allocated;
	cfg_char *str, *ptr;
	cfg_section_t *section;
	cfg_entry_t *entry;

	sz = 1;
	allocated = 512;
	*out = (cfg_char *)malloc(allocated);
	*out[0] = '\0'; /* ensure an empty '\0' terminated buffer */

	for (i = 0; i < st->nsections; i++) {
		if (i) { /* skip the root section name */
			if (st->verbose > 0)
				fprintf(stderr, "%s writing section header %d\n", fname, i);
			str = cfg_escape(st->section[i].name, &n);
			n += 3; /* [, ], \n */
			if (sz + n > allocated) {
				allocated <<= 1;
				*out = (cfg_char *)realloc(*out, allocated);
				if (!*out)
					CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
			}
			ptr = *out + sz - 1;
			sz += n;
			strcat(ptr, "[");
			strcat(ptr + 1, str);
			strcat(ptr + n - 2, "]\n");
			free(str);
		}
		section = &st->section[i];
		for (j = 0; j < section->nentries; j++) {
			if (st->verbose > 0)
				fprintf(stderr, "%s writing section %d, entry %d\n", fname, i, j);
			entry = &section->entry[j];
			str = cfg_entry_string(entry, &n);
			if (!str)
				CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
			if (sz + n > allocated) {
				allocated <<= 1;
				*out = (cfg_char *)realloc(*out, allocated);
				if (!*out)
					CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
			}
			ptr = *out + sz - 1;
			sz += n;
			strcat(ptr, str);
			free(str);
		}
	}
	*out = (cfg_char *)realloc(*out, sz); /* trim */
	if (!*out)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
	*len = sz - 1; /* exclude the '\0' character */
	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_status_t cfg_file_ptr_write(cfg_t *st, FILE *f, cfg_bool close)
{
	cfg_char *buf;
	cfg_uint32 sz = 0, sz_write = 0;
	cfg_status_t ret;

	CFG_CHECK_ST_RETURN(st, "cfg_file_ptr_write", CFG_ERROR_NULL_PTR);
	if (!f)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_FILE);

	ret = cfg_buffer_write(st, &buf, &sz);
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

cfg_status_t cfg_file_write(cfg_t *st, cfg_char *filename)
{
	FILE *f;
	CFG_CHECK_ST_RETURN(st, "cfg_file_write", CFG_ERROR_NULL_PTR);
	/* read file */
	f = fopen(filename, "w");
	return cfg_file_ptr_write(st, f, CFG_TRUE);
}
