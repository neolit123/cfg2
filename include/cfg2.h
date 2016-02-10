/*
 * cfg2
 * a simplistic configuration parser for INI like syntax in C
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
#define CFG_VERSION_MINOR 34
#define CFG_VERSION_PATCH 0

/* preprocessor definitions */
#define CFG_TRUE 1
#define CFG_FALSE 0
#define CFG_CACHE_SIZE 32
#define CFG_SEPARATOR_SECTION 0x01
#define CFG_SEPARATOR_KEY_VALUE 0x02
#define CFG_COMMENT_CHAR1 ';'
#define CFG_COMMENT_CHAR2 '#'
#define CFG_HASH_SEED 0x811c9dc5
#define CFG_ROOT_SECTION NULL
#define CFG_ROOT_SECTION_HASH CFG_HASH_SEED

#ifdef CFG_DYNAMIC
#	ifdef _WIN32
#		define CFG_EXPORT __declspec(dllexport)
#	else
#		define CFG_EXPORT __attribute__((visibility("default")))
#	endif
#else
#	define CFG_EXPORT
#endif

/* typedefs
 * NOTE: stdint.h and fixed width types are a C99 feature, but most compilers
 * should support them.
 */
#ifdef _MSC_VER
	typedef unsigned __int32 cfg_uint32;
	typedef __int32 cfg_int;
	typedef __int64 cfg_long;
#else
	#include <stdint.h>
	typedef uint32_t cfg_uint32;
	typedef int32_t cfg_int;
	typedef int64_t cfg_long;
#endif

typedef char cfg_char;
typedef unsigned char cfg_uchar;
typedef unsigned int cfg_bool;
typedef float cfg_float;
typedef double cfg_double;

/* -----------------------------------------------------------------------------
 * status enumeration; if a function returns something other than CFG_STATUS_OK,
 * an error has occured.
*/

typedef enum {
	/* 0  */ CFG_STATUS_OK,
	/* 1  */ CFG_ERROR_NULL_KEY,
	/* 2  */ CFG_ERROR_NULL_ENTRY,
	/* 3  */ CFG_ERROR_NULL_PTR,
	/* 4  */ CFG_ERROR_ALLOC,
	/* 5  */ CFG_ERROR_FREAD,
	/* 6  */ CFG_ERROR_FWRITE,
	/* 7  */ CFG_ERROR_FILE,
	/* 8  */ CFG_ERROR_INIT,
	/* 9  */ CFG_ERROR_ENTRY_NOT_FOUND,
	/* 10 */ CFG_ERROR_NO_ENTRIES,
	/* 11 */ CFG_ERROR_CACHE_SIZE
} cfg_status_t;

/* -----------------------------------------------------------------------------
 * objects
*/

/* the main library object */
typedef struct cfg_private cfg_t;

/* the library's section object */
typedef struct cfg_section_private cfg_section_t;


/* the library's data entry */
typedef struct cfg_entry_private cfg_entry_t;

/* -----------------------------------------------------------------------------
 * buffer & file I/O
*/

/* allocates a new library object */
CFG_EXPORT
cfg_t *cfg_alloc(void);

/* get the last status of the library object */
CFG_EXPORT
cfg_status_t cfg_status_get(cfg_t *st);

/* free all memory allocated by the library for a cfg_t object */
CFG_EXPORT
cfg_status_t cfg_free(cfg_t *st);

/* parse a buffer (buf) of size (sz); optional copy (copy) or work in place. */
CFG_EXPORT
cfg_status_t cfg_buffer_parse(cfg_t *st, cfg_char *buf, cfg_uint32 sz, cfg_bool copy);

/* parse a file by name, passed as the 2nd parameter. non-safe for Win32's
 * UTF-16 paths! use cfg_parse_buffer() or cfg_parse_file_ptr() instead. */
CFG_EXPORT
cfg_status_t cfg_file_parse(cfg_t *st, cfg_char *filename);

/* alternative to cfg_parse_file() that accepts a FILE* stream;
 * third argument is optional close of the stream. */
CFG_EXPORT
cfg_status_t cfg_file_ptr_parse(cfg_t *st, FILE *f, cfg_bool close);

/* write all the sections and keys to a string buffer; allocates memory at
 * the 'out' pointer and stores the length in 'len'. */
CFG_EXPORT
cfg_status_t cfg_buffer_write(cfg_t *st, cfg_char **out, cfg_uint32 *len);

/* write all the sections and keys to a file. non-safe for Win32's
 * UTF-16 paths! use cfg_write_buffer() or cfg_write_file_ptr() instead. */
CFG_EXPORT
cfg_status_t cfg_file_write(cfg_t *st, cfg_char *filename);

/* write all the sections and keys to a FILE pointer with optional close
 * when done. */
CFG_EXPORT
cfg_status_t cfg_file_ptr_write(cfg_t *st, FILE *f, cfg_bool close);

/* set the verbose level for the library object; level = 0 (OFF), 1, 2, 3... */
CFG_EXPORT
cfg_status_t cfg_verbose_set(cfg_t *st, cfg_uint32 level);

/* -----------------------------------------------------------------------------
 * cache
*/

/* set the size of the cache (2nd parameter). note that this also clears
 * the cache */
CFG_EXPORT
cfg_status_t cfg_cache_size_set(cfg_t *st, cfg_uint32 size);

/* clear the cache */
CFG_EXPORT
cfg_status_t cfg_cache_clear(cfg_t *st);

/* add an entry to the cache */
CFG_EXPORT
cfg_status_t cfg_cache_entry_add(cfg_t *st, cfg_entry_t *entry);

/* -----------------------------------------------------------------------------
 * entries
*/

/* retrieve the nth entry */
CFG_EXPORT
cfg_entry_t *cfg_entry_nth(cfg_t *st, cfg_uint32 n);

/* return an entry from section (2nd argument, can be CFG_ROOT_SECTION) and
 * key (3rd argument) */
CFG_EXPORT
cfg_entry_t *cfg_entry_get(cfg_t *st, cfg_char *section, cfg_char *key);

/* return an entry from the root section */
CFG_EXPORT
cfg_entry_t *cfg_root_entry_get(cfg_t *st, cfg_char *key);

/* get the value for an entry */
CFG_EXPORT
cfg_char *cfg_entry_value_get(cfg_t *st, cfg_entry_t *entry);

/* set a value for an entry */
CFG_EXPORT
cfg_status_t cfg_entry_value_set(cfg_t *st, cfg_entry_t *entry, cfg_char *value);

/* retrieve a specific value by section and key */
CFG_EXPORT
cfg_char *cfg_value_get(cfg_t *st, cfg_char *section, cfg_char *key);

/* retrieve a specific value by key in the root section */
CFG_EXPORT
cfg_char *cfg_root_value_get(cfg_t *st, cfg_char *key);

/* set a value for a specific key in a section; add the key if missing. */
CFG_EXPORT
cfg_status_t cfg_value_set(cfg_t *st, cfg_char *section, cfg_char *key, cfg_char *value, cfg_bool add);

/* set a value for a specific key in the root section; add the key if missing. */
CFG_EXPORT
cfg_status_t cfg_root_value_set(cfg_t *st, cfg_char *key, cfg_char *value, cfg_bool add);

/* delete an entry */
CFG_EXPORT
cfg_status_t cfg_entry_delete(cfg_t *st, cfg_entry_t *entry);

/* delete a section and all entries associated with it.
 * if the section is CFG_ROOT_SECTION only the entries will be deleted.
 * a potentially slow operation! */
CFG_EXPORT
cfg_status_t cfg_section_delete(cfg_t *st, cfg_char *section);

/* delete all entries and sections */
CFG_EXPORT
cfg_status_t cfg_clear(cfg_t *st);

/* -----------------------------------------------------------------------------
 * utilities
*/

/* string -> number conversations */
CFG_EXPORT
cfg_bool cfg_value_to_bool(cfg_char *value);
CFG_EXPORT
cfg_int cfg_value_to_int(cfg_char *value);
CFG_EXPORT
cfg_long cfg_value_to_long(cfg_char *value);
CFG_EXPORT
cfg_float cfg_value_to_float(cfg_char *value);
CFG_EXPORT
cfg_double cfg_value_to_double(cfg_char *value);

/* number -> string conversations (allocate memory) */
CFG_EXPORT
cfg_char *cfg_bool_to_value(cfg_bool number);
CFG_EXPORT
cfg_char *cfg_int_to_value(cfg_int number);
CFG_EXPORT
cfg_char *cfg_long_to_value(cfg_long number);
CFG_EXPORT
cfg_char *cfg_float_to_value(cfg_float number);
CFG_EXPORT
cfg_char *cfg_double_to_value(cfg_double number);

/* fast fnv-32 hash of a string */
CFG_EXPORT
cfg_uint32 cfg_hash_get(cfg_char *str);

/* HEX string <-> char* buffer conversations; allocates memory!
 * you can pass NULL as the first argument to ignore the 'verbose' mode of
 * cfg_t and not print anything to stderr. */
CFG_EXPORT
cfg_char *cfg_hex_to_char(cfg_t *st, cfg_char *value);
CFG_EXPORT
cfg_char *cfg_char_to_hex(cfg_t *st, cfg_char *value);

/* a local strdup() implementation */
CFG_EXPORT
cfg_char *cfg_strdup(cfg_char *str);

/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* CFG2_H */
