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

#ifndef CFG2_H
#define CFG2_H

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */

#include <stdio.h>

/* version macros */
#define CFG_VERSION_MAJOR 0
#define CFG_VERSION_MINOR 28
#define CFG_VERSION_PATCH 0

/* preprocessor definitions */
#define CFG_TRUE 1
#define CFG_FALSE 0
#define CFG_CACHE_SIZE 32
#define CFG_SECTION_SEPARATOR 0x01
#define CFG_KEY_VALUE_SEPARATOR 0x02
#define CFG_COMMENT_CHAR1 ';'
#define CFG_COMMENT_CHAR2 '#'
#define CFG_HASH_SEED 0x811c9dc5
#define CFG_ROOT_SECTION NULL
#define CFG_ROOT_SECTION_HASH CFG_HASH_SEED

/* typedefs */
#ifdef _MSC_VER
	typedef unsigned __int32 cfg_uint32;
#else
	#include <stdint.h>
	typedef uint32_t cfg_uint32;
#endif

typedef char cfg_char;
typedef unsigned char cfg_bool;
typedef float cfg_float;
typedef double cfg_double;
typedef int cfg_int;
typedef long cfg_long;
typedef unsigned char cfg_uchar;
typedef unsigned int cfg_uint;
typedef unsigned long cfg_ulong;

/* -----------------------------------------------------------------------------
 * status enumeration; if a function returns something other than CFG_STATUS_OK,
 * an error has occured.
*/

typedef enum {
	/* 0  */ CFG_STATUS_OK,
	/* 1  */ CFG_ERROR_NULL_KEY,
	/* 2  */ CFG_ERROR_NULL_PTR,
	/* 3  */ CFG_ERROR_ALLOC,
	/* 4  */ CFG_ERROR_CRITICAL,
	/* 5  */ CFG_ERROR_FREAD,
	/* 6  */ CFG_ERROR_FWRITE,
	/* 7  */ CFG_ERROR_FOPEN,
	/* 8  */ CFG_ERROR_INIT,
	/* 9  */ CFG_ERROR_KEY_NOT_FOUND,
	/* 10 */ CFG_ERROR_PARSE,
	/* 11 */ CFG_ERROR_NO_KEYS
} cfg_status_t;

/* -----------------------------------------------------------------------------
 * the library object entry. holds a pair of key / value.
*/

typedef struct {
	cfg_uint32 key_hash;
	cfg_uint32 section_hash;
	cfg_char *key;
	cfg_char *value;
	cfg_uint32 index;
} cfg_entry_t;

/* -----------------------------------------------------------------------------
 * the main library object
*/

typedef struct {
	cfg_entry_t **cache;
	cfg_uint32 cache_size;

	cfg_entry_t *entry;
	cfg_char **section;
	cfg_char *buf;
	FILE *file;

	cfg_bool init;
	cfg_uint32 verbose;
	cfg_uint32 nentries;
	cfg_uint32 nsections;
	cfg_uint32 buf_size;
	cfg_char section_separator;
	cfg_char key_value_separator;
	cfg_char comment_char1;
	cfg_char comment_char2;
} cfg_t;

/* -----------------------------------------------------------------------------
 * buffer & file I/O
*/

/* allocates a new cfg_t object; if 'init' == CFG_TRUE, cfg_init() will also be
 * called */
cfg_t *cfg_alloc(cfg_bool init);

/* init the library object. must be called before everything else. */
cfg_status_t cfg_init(cfg_t *st);

/* free all memory allocated by the library for a cfg_t object,
 * if you pass 'free_ptr' == CFG_TRUE then 'st' will be freed as well! */
cfg_status_t cfg_free(cfg_t *st, cfg_bool free_ptr);

/* parse a buffer (buf) of size (sz); optional copy (copy) or work in place. */
cfg_status_t cfg_parse_buffer(cfg_t *st, cfg_char *buf, cfg_uint32 sz, cfg_bool copy);

/* parse a file by name, passed as the 2nd parameter. non-safe for Win32's
 * UTF-16 paths! use cfg_parse_buffer() or cfg_parse_file_ptr() instead. */
cfg_status_t cfg_parse_file(cfg_t *st, cfg_char *filename);

/* alternative to cfg_parse_file() that accepts a FILE* stream;
 * third argument is optional close of the stream. */
cfg_status_t cfg_parse_file_ptr(cfg_t *st, FILE *f, cfg_bool close);

/* write all the sections and keys to a string buffer; allocates memory at
 * the 'out' pointer and stores the length in 'len'. */
cfg_status_t cfg_write_buffer(cfg_t *st, cfg_char **out, cfg_uint32 *len);

/* write all the sections and keys to a file */
cfg_status_t cfg_write_file(cfg_t *st, cfg_char *filename);

/* write all the sections and keys to a FILE pointer with optional close
 * when done. */
cfg_status_t cfg_write_file_ptr(cfg_t *st, FILE *f, cfg_bool close);

/* -----------------------------------------------------------------------------
 * cache
*/

/* set the size of the cache (2nd parameter). note that this also clears
 * the cache */
cfg_status_t cfg_cache_size_set(cfg_t *st, cfg_uint32 size);

/* clear the cache */
cfg_status_t cfg_cache_clear(cfg_t *st);

/* add an entry to the cache */
cfg_status_t cfg_cache_entry_add(cfg_t *st, cfg_entry_t *entry);

/* -----------------------------------------------------------------------------
 * entries
*/

/* retrieve the nth entry */
cfg_entry_t *cfg_entry_nth(cfg_t *st, cfg_uint32 n);

/* return an entry from section (2nd argument, can be CFG_ROOT_SECTION) and
 * key (3rd argument) */
cfg_entry_t *cfg_entry_get(cfg_t *st, cfg_char *section, cfg_char *key);

/* return an entry from the root section */
cfg_entry_t *cfg_root_entry_get(cfg_t *st, cfg_char *key);

/* get the value for an entry */
cfg_char *cfg_entry_value_get(cfg_t *st, cfg_entry_t *entry);

/* set a value for an entry */
cfg_status_t cfg_entry_value_set(cfg_t *st, cfg_entry_t *entry, cfg_char *value);

/* retrieve a specific value by section and key */
cfg_char *cfg_value_get(cfg_t *st, cfg_char *section, cfg_char *key);

/* retrieve a specific value by key in the root section */
cfg_char *cfg_root_value_get(cfg_t *st, cfg_char *key);

/* set a value for a specific key in a section; add the key if missing. */
cfg_status_t cfg_value_set(cfg_t *st, cfg_char *section, cfg_char *key, cfg_char *value, cfg_bool add);

/* set a value for a specific key in the root section; add the key if missing. */
cfg_status_t cfg_root_value_set(cfg_t *st, cfg_char *key, cfg_char *value, cfg_bool add);

/* -----------------------------------------------------------------------------
 * utilities
*/

/* string -> number conversations */
cfg_bool cfg_value_to_bool(cfg_char *value);
cfg_long cfg_value_to_long(cfg_char *value);
cfg_ulong cfg_value_to_ulong(cfg_char *value);
cfg_double cfg_value_to_double(cfg_char *value);

/* number -> string conversations (allocate memory) */
cfg_char *cfg_bool_to_value(cfg_bool number);
cfg_char *cfg_long_to_value(cfg_long number);
cfg_char *cfg_ulong_to_value(cfg_ulong number);
cfg_char *cfg_double_to_value(cfg_double number);

/* fast fnv-32 hash of a string */
cfg_uint32 cfg_hash_get(cfg_char *str);

/* HEX string <-> char* buffer conversations; allocates memory!
 * you can pass NULL as the first argument to ignore the 'verbose' mode of
 * cfg_t and not print anything to stderr. */
cfg_char *cfg_hex_to_char(cfg_t *st, cfg_char *value);
cfg_char *cfg_char_to_hex(cfg_t *st, cfg_char *value);

/* a local strdup() implementation */
cfg_char *cfg_strdup(cfg_char *str);

/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* CFG2_H */
