/*
 * cfg2
 * a simplistic configuration parser for INI like syntax in C
 *
 * author: lubomir i. ivanov (neolit123 at gmail)
 * this code is released in the public domain without warranty of any kind.
 * providing credit to the original author is recommended but not mandatory.
 *
 * cache.c:
 *	cache related functions
 */

#include "defines.h"

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

	/* check if we are setting the buffer to zero length */
	if (!size) {
		if (st->cache)
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

cfg_status_t cfg_cache_entry_add(cfg_t *st, cfg_entry_t *entry)
{
	CFG_CHECK_ST_RETURN(st, "cfg_cache_entry_add", CFG_ERROR_NULL_PTR);
	if (!entry)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_NULL_PTR);
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
