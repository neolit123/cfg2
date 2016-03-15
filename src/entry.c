/*
 * cfg2
 * a simplistic configuration parser for INI like syntax in C
 *
 * author: lubomir i. ivanov (neolit123 at gmail)
 * this code is released in the public domain without warranty of any kind.
 * providing credit to the original author is recommended but not mandatory.
 *
 * entry.c:
 *	function for sections and entries
 */

#include "defines.h"

/* not exposed in the API */
cfg_entry_t *cfg_cache_entry_get(cfg_t *st, cfg_uint32 section_hash, cfg_uint32 key_hash);
void cfg_cache_entry_delete(cfg_t *st, cfg_entry_t *entry);

cfg_uint32 cfg_total_sections(cfg_t *st)
{
	CFG_CHECK_ST_RETURN(st, "cfg_total_sections", 0);
	return st->nsections;
}

cfg_section_t *cfg_section_nth(cfg_t *st, cfg_uint32 n)
{
	CFG_CHECK_ST_RETURN(st, "cfg_section_nth", NULL);
	if (n > st->nsections - 1) {
		CFG_SET_STATUS(st, CFG_ERROR_OUT_OF_RANGE);
		return NULL;
	}
	return &st->section[n];
}

cfg_char *cfg_section_name_get(cfg_t *st, cfg_section_t *section)
{
	CFG_CHECK_ST_RETURN(st, "cfg_section_name_get", NULL);
	if (!section) {
		CFG_SET_STATUS(st, CFG_ERROR_NULL_PTR);
		return 0;
	}
	return section->name;
}

cfg_uint32 cfg_total_entries(cfg_t *st, cfg_section_t *section)
{
	CFG_CHECK_ST_RETURN(st, "cfg_total_entries", 0);
	if (!section) {
		CFG_SET_STATUS(st, CFG_ERROR_NULL_PTR);
		return 0;
	}
	return section->nentries;
}

cfg_entry_t *cfg_entry_nth(cfg_t *st, cfg_section_t *section, cfg_uint32 n)
{
	CFG_CHECK_ST_RETURN(st, "cfg_section_nth", NULL);
	if (!section) {
		CFG_SET_STATUS(st, CFG_ERROR_NULL_PTR);
		return 0;
	}
	if (n > section->nentries - 1) {
		CFG_SET_STATUS(st, CFG_ERROR_OUT_OF_RANGE);
		return NULL;
	}
	return &section->entry[n];
}

cfg_section_t *cfg_section_get(cfg_t *st, const cfg_char *section)
{
	cfg_uint32 i, section_hash;

	CFG_CHECK_ST_RETURN(st, "cfg_section_get", NULL);
	if (section == CFG_ROOT_SECTION) {
		CFG_SET_STATUS(st, CFG_STATUS_OK);
		return &st->section[0];
	} else {
		section_hash = cfg_hash_get(section);
	}

	for (i = 1; i < st->nsections; i++) {
		if (st->section[i].hash != section_hash)
			continue;
		CFG_SET_STATUS(st, CFG_STATUS_OK);
		return &st->section[i];
	}
	CFG_SET_STATUS(st, CFG_ERROR_NOT_FOUND);
	return NULL;
}

cfg_entry_t *cfg_entry_get(cfg_t *st, const cfg_char *section, const cfg_char *key)
{
	cfg_section_t *section_ptr;
	cfg_uint32 key_hash, i;
	cfg_entry_t *entry;

	CFG_CHECK_ST_RETURN(st, "cfg_entry_get", NULL);
	if (!key) {
		CFG_SET_STATUS(st, CFG_ERROR_NULL_PTR);
		return NULL;
	}

	section_ptr = cfg_section_get(st, section);
	if (!section_ptr)
		return NULL;

	key_hash = cfg_hash_get(key);

	/* check for value in cache first */
	entry = cfg_cache_entry_get(st, section_ptr->hash, key_hash);
	if (entry)
		return entry;

	for (i = 0; i < section_ptr->nentries; i++) {
		entry = &section_ptr->entry[i];
		if (key_hash != entry->key_hash)
			continue;
		cfg_cache_entry_add(st, entry);
		CFG_SET_STATUS(st, CFG_STATUS_OK);
		return entry;
	}
	CFG_SET_STATUS(st, CFG_ERROR_NOT_FOUND);
	return NULL;
}

cfg_entry_t *cfg_root_entry_get(cfg_t *st, const cfg_char *key)
{
	return cfg_entry_get(st, CFG_ROOT_SECTION, key);
}

cfg_entry_t *cfg_entry_add(cfg_t *st, const cfg_char *section, const cfg_char *key, const cfg_char *value)
{
	cfg_uint32 key_hash;
	cfg_entry_t *entry;
	cfg_section_t *section_ptr;

	CFG_CHECK_ST_RETURN(st, "cfg_entry_add", NULL);

	entry = cfg_entry_get(st, section, key);
	if (entry) {
		cfg_entry_value_set(st, entry, value);
		return entry;
	}

	key_hash = cfg_hash_get(key);
	section_ptr = cfg_section_get(st, section);

	/* create a new section and add the entry to it */
	if (!section_ptr) {
		st->section = (cfg_section_t *)realloc(st->section, (st->nsections + 1) * sizeof(cfg_section_t));
		if (!st->section) {
			CFG_SET_STATUS(st, CFG_ERROR_ALLOC);
			return NULL;
		}
		section_ptr = &st->section[st->nsections];
		section_ptr->name = cfg_strdup(section);
		section_ptr->hash = cfg_hash_get(section);
		section_ptr->nentries = 1;
		section_ptr->entry = (cfg_entry_t *)malloc(sizeof(cfg_entry_t));
		entry = &section_ptr->entry[0];
		entry->section = section_ptr;
		entry->key = cfg_strdup(key);
		entry->key_hash = key_hash;
		entry->value = cfg_strdup(value);
		st->nsections++;
		cfg_cache_entry_add(st, entry);
		CFG_SET_STATUS(st, CFG_STATUS_OK);
		return entry;
	}

	/* add entry to an existing section */
	section_ptr->entry = (cfg_entry_t *)realloc(section_ptr->entry, (section_ptr->nentries + 1) * sizeof(cfg_section_t));
	if (!section_ptr->entry) {
		CFG_SET_STATUS(st, CFG_ERROR_ALLOC);
		return NULL;
	}
	entry = &section_ptr->entry[section_ptr->nentries];
	entry->section = section_ptr;
	entry->key = cfg_strdup(key);
	entry->key_hash = key_hash;
	entry->value = cfg_strdup(value);
	section_ptr->nentries++;
	CFG_SET_STATUS(st, CFG_STATUS_OK);
	return entry;
}

cfg_entry_t *cfg_root_entry_add(cfg_t *st, const cfg_char *key, const cfg_char *value)
{
	return cfg_entry_add(st, CFG_ROOT_SECTION, key, value);
}

cfg_char *cfg_value_get(cfg_t *st, const cfg_char *section, const cfg_char *key)
{
	cfg_entry_t *entry = cfg_entry_get(st, section, key);
	return entry ? entry->value : NULL;
}

cfg_char *cfg_root_value_get(cfg_t *st, const cfg_char *key)
{
	return cfg_value_get(st, CFG_ROOT_SECTION, key);
}

cfg_char *cfg_entry_key_get(cfg_t *st, cfg_entry_t *entry)
{
	CFG_CHECK_ST_RETURN(st, "cfg_entry_key_get", NULL);
	if (!entry) {
		CFG_SET_STATUS(st, CFG_ERROR_NULL_PTR);
		return NULL;
	}
	return entry->key;
}

cfg_char *cfg_entry_value_get(cfg_t *st, cfg_entry_t *entry)
{
	CFG_CHECK_ST_RETURN(st, "cfg_entry_value_get", NULL);
	if (!entry) {
		CFG_SET_STATUS(st, CFG_ERROR_NULL_PTR);
		return NULL;
	}
	return entry->value;
}

cfg_status_t cfg_entry_value_set(cfg_t *st, cfg_entry_t *entry, const cfg_char *value)
{
	CFG_CHECK_ST_RETURN(st, "cfg_entry_value_set", CFG_ERROR_NULL_PTR);
	if (!entry || !value)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_NULL_PTR);
	free(entry->value);
	entry->value = cfg_strdup(value);
	if (!entry->value)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_status_t cfg_value_set(cfg_t *st, const cfg_char *section, const cfg_char *key, const cfg_char *value, cfg_bool add)
{
	cfg_uint32 i, key_hash;
	cfg_entry_t *entry;
	cfg_section_t *section_ptr;

	CFG_CHECK_ST_RETURN(st, "cfg_value_set", CFG_ERROR_NULL_PTR);

	if (!key || !value)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_NULL_PTR);

	if (add) {
		entry = cfg_entry_add(st, section, key, value);
		return st->status;
	}

	key_hash = cfg_hash_get(key);
	section_ptr = cfg_section_get(st, section);
	if (!section_ptr)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_NOT_FOUND);

	/* look for the entry in the existing section */
	for (i = 0; i < section_ptr->nentries; i++) {
		entry = &section_ptr->entry[i];
		if (key_hash == entry->key_hash) {
			cfg_cache_entry_add(st, entry);
			return cfg_entry_value_set(st, entry, value);
		}
	}

	CFG_SET_RETURN_STATUS(st, CFG_ERROR_NOT_FOUND);
}

cfg_status_t cfg_root_value_set(cfg_t *st, const cfg_char *key, const cfg_char *value, cfg_bool add)
{
	return cfg_value_set(st, CFG_ROOT_SECTION, key, value, add);
}

cfg_status_t cfg_entry_delete(cfg_t *st, cfg_entry_t *entry)
{
	cfg_uint32 idx;
	cfg_section_t *section;

	CFG_CHECK_ST_RETURN(st, "cfg_entry_delete", CFG_ERROR_NULL_PTR);
	if (!entry)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_NULL_PTR);

	section = entry->section;
	free(entry->key);
	free(entry->value);

	/* delete from the cache */
	cfg_cache_entry_delete(st, entry);

	idx = entry - &section->entry[0];
	if (idx < section->nentries - 1)
		memcpy((void *)&section->entry[idx], (void *)&section->entry[idx + 1], (section->nentries - idx - 1) * sizeof(cfg_entry_t));

	section->nentries--;
	if (section->nentries) {
		section->entry = (cfg_entry_t *)realloc(section->entry, section->nentries * sizeof(cfg_entry_t));
		if (!section->entry)
			CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
	} else {
		free(section->entry);
		section->entry = NULL;
	}

	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}

cfg_status_t cfg_section_delete(cfg_t *st, const cfg_char *section)
{
	cfg_uint32 i, idx;
	cfg_section_t *section_ptr;
	cfg_entry_t *entry;

	CFG_CHECK_ST_RETURN(st, "cfg_section_delete", CFG_ERROR_NULL_PTR);

	section_ptr = cfg_section_get(st, section);
	if (!section_ptr)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_NOT_FOUND);

	for (i = 0; i < section_ptr->nentries; i++) {
		entry = &section_ptr->entry[i];
		free(entry->key);
		free(entry->value);
	}

	free(section_ptr->name);
	free(section_ptr->entry);

	idx = section_ptr - &st->section[0];
	if (idx < st->nsections - 1)
		memcpy((void *)&st->section[idx], (void *)&st->section[idx + 1], (st->nsections - idx - 1) * sizeof(cfg_section_t));
	st->nsections--;
	st->section = (cfg_section_t *)realloc(st->section, st->nsections * sizeof(cfg_section_t));
	if (!st->section)
		CFG_SET_RETURN_STATUS(st, CFG_ERROR_ALLOC);
	CFG_SET_RETURN_STATUS(st, CFG_STATUS_OK);
}
