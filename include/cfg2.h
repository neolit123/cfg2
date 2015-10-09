/*
 * cfg2
 * a simplistic configuration parser for INI like syntax in ANSI C
 *
 * author: lubomir i. ivanov (neolit123 at gmail)
 * this code is released in the public domain without warranty of any kind.
 * providing credit to the original author is recommended but not mandatory.
 *
 * cfg2.h:
 *	this is the library header
 */

/* preprocessor definitions and macros */
#define CFG_TRUE 1
#define CFG_FALSE 0
#define CFG_CACHE_SIZE 32
#define CFG_KEY_VALUE_SEPARATOR 0x01
#define CFG_SECTION_SEPARATOR 0x02
#define CFG_COMMENT_CHAR ';'
#define CFG_ROOT_SECTION NULL
#define CFG_ROOT_SECTION_HASH 0

#define CFG_VERSION_MAJOR 0
#define CFG_VERSION_MINOR 15
#define CFG_VERSION_PATCH 0

#ifdef _MSC_VER
	typedef unsigned __int32 cfg_uint32t;
#else
	#include <stdint.h>
	typedef uint32_t cfg_uint32;
#endif

typedef char cfg_char;
typedef float cfg_float;
typedef double cfg_double;
typedef int cfg_int;
typedef long cfg_long;
typedef unsigned int cfg_uint;
typedef unsigned long cfg_ulong;

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

/* an entry pair of key / value */
struct cfg_entry {
	cfg_uint32 key_hash;
	cfg_uint32 value_hash;
	cfg_uint32 section_hash;
	cfg_char *key;
	cfg_char *value;
	cfg_uint32 index;
};

typedef struct cfg_entry cfg_entry_t;

/* the main library object */
typedef struct {
	cfg_entry_t **cache;
	cfg_uint32 cache_size;

	cfg_entry_t *entry;
	cfg_char *buf;
	FILE *file;

	cfg_uint32 verbose;
	cfg_uint32 init;
	cfg_uint32 nkeys;
	cfg_uint32 nsections;
	cfg_uint32 buf_size;
	cfg_char key_value_separator;
	cfg_char section_separator;
	cfg_char comment_char;
} cfg_t;

/* init the library object. must be called before everything else. */
cfg_error_t cfg_init(cfg_t*, cfg_uint32 unused);

/* free all memory allocated by the library for a cfg_t object */
cfg_error_t cfg_free(cfg_t*);

/* parse a NULL terminated char buffer.
 * the third argument is size, but is redundant (deprecated). */
cfg_error_t cfg_parse_buffer(cfg_t*, cfg_char*, cfg_uint32 unused);

/* parse a file by name, passed as the 2nd parameter */
cfg_error_t cfg_parse_file(cfg_t*, cfg_char*);

/* set the size of the cache (2nd parameter). note that this also clears
 * the cache */
cfg_error_t cfg_cache_size_set(cfg_t*, cfg_uint32);

/* clear the cache */
void cfg_cache_clear(cfg_t*);

/* retrieve the nth entry */
cfg_entry_t *cfg_entry_nth(cfg_t*, cfg_uint32);

/* return a value from section (2nd argument) and key (3rd argument) */
cfg_entry_t *cfg_section_entry_get(cfg_t*, cfg_char*, cfg_char*);

/* retrieve the nth key */
cfg_char *cfg_key_nth(cfg_t*, cfg_uint32);

/* retrieve a key from key value */
cfg_char *cfg_key_get(cfg_t*, cfg_char*);

/* get the list index of a key */
cfg_uint32 cfg_key_get_index(cfg_t*, cfg_char*);

/* retrieve the nth value */
cfg_char *cfg_value_nth(cfg_t*, cfg_uint32);

/* retrieve a specific value by key */
cfg_char *cfg_value_get(cfg_t*, cfg_char*);

/* retrieve a specific value by key as unsigned long integer.
 * third value is integer base (2, 10, 16 etc.) */
cfg_ulong cfg_value_get_ulong(cfg_t*, cfg_char*, cfg_int);

/* retrieve a specific value by key as signed long integer.
 * third value is integer base (2, 10, 16 etc.) */
cfg_long cfg_value_get_long(cfg_t*, cfg_char*, cfg_int);

/* retrieve a specific value by key as double precision floating point. */
cfg_double cfg_value_get_double(cfg_t*, cfg_char*);

/* set a value (3rd argument) for a specific key (2nd argument) */
cfg_error_t cfg_value_set(cfg_t*, cfg_char*, cfg_char*);

/* same as the ones above except here you need to feed cfg_entry_t pointers */
cfg_ulong cfg_entry_value_get_ulong(cfg_t*, cfg_entry_t*, cfg_int);
cfg_long cfg_entry_value_get_long(cfg_t*, cfg_entry_t*, cfg_int);
cfg_double cfg_entry_value_get_double(cfg_t*, cfg_entry_t*);

/* add an entry to the cache */
cfg_error_t cfg_cache_entry_add(cfg_t*, cfg_entry_t*);
