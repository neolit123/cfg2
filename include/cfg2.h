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
#define CFG_VERSION_MINOR 99
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

/* if you want to link statically on win32, define CFG_LIB_STATIC before
 * including this header. */
#if defined(_WIN32) || defined(__CYGWIN__)
#	if defined(CFG_LIB_BUILD) && defined(CFG_LIB_DYNAMIC)
#		define CFG_API __declspec(dllexport)
#	elif defined(CFG_LIB_BUILD) || defined(CFG_LIB_STATIC)
#		define CFG_API
#	else
#		define CFG_API __declspec(dllimport)
#	endif
#elif defined(CFG_LIB_BUILD)
#	define CFG_API __attribute__((visibility("default")))
#else
#	define CFG_API
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
	/* 1  */ CFG_ERROR_NULL_PTR,
	/* 2  */ CFG_ERROR_ALLOC,
	/* 3  */ CFG_ERROR_FREAD,
	/* 4  */ CFG_ERROR_FWRITE,
	/* 5  */ CFG_ERROR_FILE,
	/* 6  */ CFG_ERROR_NOT_FOUND,
	/* 7  */ CFG_ERROR_OUT_OF_RANGE,
	/* 8  */ CFG_ERROR_CACHE_SIZE
} cfg_status_t;

/* -----------------------------------------------------------------------------
 * objects
*/

/* the main library object */
typedef struct _cfg_t cfg_t;

/* the library's section object */
typedef struct _cfg_section_t cfg_section_t;

/* the library's data entry */
typedef struct _cfg_entry_t cfg_entry_t;

/* -----------------------------------------------------------------------------
 * buffer & file I/O
*/

/* allocates a new library object */
CFG_API
cfg_t *cfg_alloc(void);

/* free all memory allocated by the library for a cfg_t object */
CFG_API
cfg_status_t cfg_free(cfg_t *st);

/* parse a buffer (buf) of size (sz); optional copy (copy) or work in place. */
CFG_API
cfg_status_t cfg_buffer_parse(cfg_t *st, cfg_char *buf, cfg_uint32 sz, cfg_bool copy);

/* parse a file by name, passed as the 2nd parameter. non-safe for Win32's
 * UTF-16 paths! use cfg_buffer_parse() or cfg_file_ptr_parse() instead. */
CFG_API
cfg_status_t cfg_file_parse(cfg_t *st, cfg_char *filename);

/* alternative to cfg_file_parse() that accepts a FILE* stream;
 * third argument is optional close of the stream. */
CFG_API
cfg_status_t cfg_file_ptr_parse(cfg_t *st, FILE *f, cfg_bool close);

/* write all the sections and keys to a string buffer; allocates memory at
 * the 'out' pointer and stores the length in 'len'. */
CFG_API
cfg_status_t cfg_buffer_write(cfg_t *st, cfg_char **out, cfg_uint32 *len);

/* write all the sections and keys to a file. non-safe for Win32's
 * UTF-16 paths! use cfg_buffer_write() or cfg_file_ptr_write() instead. */
CFG_API
cfg_status_t cfg_file_write(cfg_t *st, cfg_char *filename);

/* write all the sections and keys to a FILE pointer with optional close
 * when done. */
CFG_API
cfg_status_t cfg_file_ptr_write(cfg_t *st, FILE *f, cfg_bool close);

/* set the verbose level for the library object; level = 0 (OFF), 1, 2, 3... */
CFG_API
cfg_status_t cfg_verbose_set(cfg_t *st, cfg_uint32 level);

/* get the last status of the library object */
CFG_API
cfg_status_t cfg_status_get(cfg_t *st);

/* -----------------------------------------------------------------------------
 * cache
*/

/* set the size of the cache (2nd parameter). note that this also clears
 * the cache */
CFG_API
cfg_status_t cfg_cache_size_set(cfg_t *st, cfg_uint32 size);

/* clear the cache */
CFG_API
cfg_status_t cfg_cache_clear(cfg_t *st);

/* add an entry to the cache */
CFG_API
cfg_status_t cfg_cache_entry_add(cfg_t *st, cfg_entry_t *entry);

/* retrieve the nth entry from the cache */
CFG_API
cfg_entry_t *cfg_cache_entry_nth(cfg_t *st, cfg_uint32 n);

/* -----------------------------------------------------------------------------
 * sections and entries
*/

/* get a section pointer; 'section' can be CFG_ROOT_SECTION */
CFG_API
cfg_section_t *cfg_section_get(cfg_t *st, const cfg_char *section);

/* get the total number of sections; the root section is always present, thus n >= 1 */
CFG_API
cfg_uint32 cfg_total_sections(cfg_t *st);

/* get the nth section; n = 0 is always the root section */
CFG_API
cfg_section_t *cfg_section_nth(cfg_t *st, cfg_uint32 n);

/* get the raw name of a section */
CFG_API
cfg_char *cfg_section_name_get(cfg_t *st, cfg_section_t *section);

/* get the total number of entries within a section */
CFG_API
cfg_uint32 cfg_total_entries(cfg_t *st, cfg_section_t *section);

/* get the nth entry from a section */
CFG_API
cfg_entry_t *cfg_entry_nth(cfg_t *st, cfg_section_t *section, cfg_uint32 n);

/* return an entry from section (2nd argument, can be CFG_ROOT_SECTION) and
 * key (3rd argument) */
CFG_API
cfg_entry_t *cfg_entry_get(cfg_t *st, const cfg_char *section, const cfg_char *key);

/* return an entry from the root section */
CFG_API
cfg_entry_t *cfg_root_entry_get(cfg_t *st, const cfg_char *key);

/* create an entry (unless the entry already exists) and return a pointer to it. */
CFG_API
cfg_entry_t *cfg_entry_add(cfg_t *st, const cfg_char *section, const cfg_char *key, const cfg_char *value);

/* create an entry in the root section (unless the entry already exists) and return a pointer to it. */
CFG_API
cfg_entry_t *cfg_root_entry_add(cfg_t *st, const cfg_char *key, const cfg_char *value);

/* get the key for an entry */
CFG_API
cfg_char *cfg_entry_key_get(cfg_t *st, cfg_entry_t *entry);

/* get the value for an entry */
CFG_API
cfg_char *cfg_entry_value_get(cfg_t *st, cfg_entry_t *entry);

/* set a value for an entry */
CFG_API
cfg_status_t cfg_entry_value_set(cfg_t *st, cfg_entry_t *entry, const cfg_char *value);

/* retrieve a specific value by section and key */
CFG_API
cfg_char *cfg_value_get(cfg_t *st, const cfg_char *section, const cfg_char *key);

/* retrieve a specific value by key in the root section */
CFG_API
cfg_char *cfg_root_value_get(cfg_t *st, const cfg_char *key);

/* set a value for a specific key in a section; add the key if missing. */
CFG_API
cfg_status_t cfg_value_set(cfg_t *st, const cfg_char *section, const cfg_char *key, const cfg_char *value, cfg_bool add);

/* set a value for a specific key in the root section; add the key if missing. */
CFG_API
cfg_status_t cfg_root_value_set(cfg_t *st, const cfg_char *key, const cfg_char *value, cfg_bool add);

/* delete an entry */
CFG_API
cfg_status_t cfg_entry_delete(cfg_t *st, cfg_entry_t *entry);

/* delete a section and all entries associated with it.
 * if the section is CFG_ROOT_SECTION only the entries will be deleted.
 * a potentially slow operation! */
CFG_API
cfg_status_t cfg_section_delete(cfg_t *st, const cfg_char *section);

/* delete all entries and sections */
CFG_API
cfg_status_t cfg_clear(cfg_t *st);

/* -----------------------------------------------------------------------------
 * utilities
*/

/* string -> number conversations */
CFG_API
cfg_bool cfg_value_to_bool(const cfg_char *value);
CFG_API
cfg_int cfg_value_to_int(const cfg_char *value);
CFG_API
cfg_long cfg_value_to_long(const cfg_char *value);
CFG_API
cfg_float cfg_value_to_float(const cfg_char *value);
CFG_API
cfg_double cfg_value_to_double(const cfg_char *value);

/* number -> string conversations (allocate memory) */
CFG_API
cfg_char *cfg_bool_to_value(cfg_bool number);
CFG_API
cfg_char *cfg_int_to_value(cfg_int number);
CFG_API
cfg_char *cfg_long_to_value(cfg_long number);
CFG_API
cfg_char *cfg_float_to_value(cfg_float number);
CFG_API
cfg_char *cfg_double_to_value(cfg_double number);

/* fast fnv-32 hash of a string */
CFG_API
cfg_uint32 cfg_hash_get(const cfg_char *str);

/* HEX string <-> char* buffer conversations; allocates memory! */
CFG_API
cfg_char *cfg_hex_to_char(const cfg_char *value);
CFG_API
cfg_char *cfg_char_to_hex(const cfg_char *value);

/* a local strdup() implementation */
CFG_API
cfg_char *cfg_strdup(const cfg_char *str);

/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* CFG2_H */
