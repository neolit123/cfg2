/*
 * cfg2 0.1
 * a simplistic configuration parser for INI like syntax in ANSI C
 *
 * author: lubomir i. ivanov (neolit123 at gmail)
 * this code is released in the public domain without warranty of any kind.
 * providing credit to the original author is recommended but not mandatory. 
 *
 * cfg2.c:
 *	this is the library header
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* preprocessor definitions and macros */
#define CFG_TRUE 1
#define CFG_FALSE 0
#define CFG_CACHE_SIZE 32

typedef char cfg_char;
typedef float cfg_float;
typedef double cfg_double;
typedef int cfg_int;
typedef long cfg_long;
typedef unsigned long cfg_ulong;
typedef unsigned int cfg_uint32;

/* list of different error types */
typedef enum {
	/* 0  */ CFG_ERROR_OK,
	/* 1  */ CFG_ERROR_NULL_KEY,
	/* 2  */ CFG_ERROR_NO_KEYS,
	/* 3  */ CFG_ERROR_ALLOC,
	/* 4  */ CFG_ERROR_CRITICAL,
	/* 5  */ CFG_ERROR_FREAD,
	/* 6  */ CFG_ERROR_FOPEN,
	/* 7  */ CFG_ERROR_INIT,
	/* 8  */ CFG_ERROR_KEY_NOT_FOUND
} cfg_error_t;

/* the main library object */
typedef struct {
	cfg_uint32 *cache_keys_hash;
	cfg_int *cache_keys_index;

	cfg_uint32 *keys_hash;
	cfg_uint32 *values_hash;
	cfg_char **keys;
	cfg_char **values;
	cfg_char *buf;
	FILE *file;

	cfg_int init;
	cfg_int nkeys;
	cfg_int buf_size;
	cfg_int cache_size;
} cfg_t;

/* init the library object. must be called before everything else */
cfg_error_t cfg_init(cfg_t*);

/* free all memory allocated by the library for a cfg_t object */
cfg_error_t cfg_free(cfg_t*);

/* parse a char buffer by passing its size as the 3rd parameter */
cfg_error_t cfg_parse_buffer(cfg_t*, cfg_char*, cfg_int);

/* parse a file by name, passed as the 2nd parameter */
cfg_error_t cfg_parse_file(cfg_t*, cfg_char*);

/* set the size of the cache (2nd parameter). note that this also clears
 * the cache */
cfg_error_t cfg_cache_size_set(cfg_t*, cfg_int);

/* clear the cache */
void cfg_cache_clear(cfg_t*);

/* retrieve the nth key */
cfg_char* cfg_key_nth(cfg_t*, cfg_int);

/* retrieve a key from key value */
cfg_char* cfg_key_get(cfg_t*, cfg_char*);

/* get the list index of a key */
cfg_int cfg_key_get_index(cfg_t*, cfg_char*);

/* retrieve the nth value */
cfg_char* cfg_value_nth(cfg_t*, cfg_int);

/* retrieve a specific value by key */
cfg_char* cfg_value_get(cfg_t*, cfg_char*);

/* retrieve a specific value by key as unsigned long integer.
 * third value is integer base (2, 10, 16 etc.) */
cfg_ulong cfg_value_get_ulong(cfg_t*, cfg_char*, cfg_int);

/* retrieve a specific value by key as signed long integer.
 * third value is integer base (2, 10, 16 etc.) */
cfg_long cfg_value_get_long(cfg_t*, cfg_char*, cfg_int);

/* retrieve a specific value by key as double precision floating point. */
cfg_double cfg_value_get_double(cfg_t*, cfg_char*);
