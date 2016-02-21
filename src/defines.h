#ifndef DEFINES_H
#define DEFINES_H

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4255)
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

struct _cfg_t {
	cfg_char separator_section;
	cfg_char separator_key_value;
	cfg_char comment_char1;
	cfg_char comment_char2;

	cfg_status_t status;
	cfg_uint32 verbose;
	cfg_uint32 cache_size;
	cfg_uint32 nsections;
	cfg_section_t *section;

	cfg_entry_t **cache;
};

struct _cfg_section_t {
	cfg_uint32 hash;
	cfg_uint32 nentries;
	cfg_char *name;
	cfg_entry_t *entry;
};

struct _cfg_entry_t {
	cfg_uint32 key_hash;
	cfg_char *key;
	cfg_char *value;
	cfg_section_t *section;
};

#endif
