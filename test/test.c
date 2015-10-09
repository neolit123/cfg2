/*
 * cfg2
 * a simplistic configuration parser for INI like syntax in ANSI C
 *
 * author: lubomir i. ivanov (neolit123 at gmail)
 * this code is released in the public domain without warranty of any kind.
 * providing credit to the original author is recommended but not mandatory.
 *
 * test.c:
 *	test file for the api
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cfg2.h"
/*
 * test parsing a file or a buffer directly. when calling a parsing method
 * the memory pointed by the cfg_t will be released automatically.
 * when you are done with the library make sure to call cfg_free().
 */
int main(void)
{
	int i, err;
	cfg_t st;
	cfg_entry_t *entry;
	char buf[] =
"key1=\\tvalue1\n" \
"key2=value2\n" \
"key3=value3\n" \
"key4=value4\n\0";

	puts("[cfg2 test]");
	puts("* init");
	/* init the structure with cache buffer size of 4. this means that 4 unique
	 * (and fast) entries will be cached at all times. */
	err = cfg_init(&st, 0);
	st.cache_size = 4;
	st.verbose = 1;
	if (err > 0) {
		printf("cfg_init() ERROR: %d\n", err);
		goto exit;
	}
	printf("* cache size: %d\n", st.cache_size);
	puts("* parse");
	err = cfg_parse_buffer(&st, buf, 0);
	err = cfg_parse_file(&st, "test.cfg");

	puts("");
	puts("* actions");
	if (err > 0) {
		printf("cfg_parse_file() ERROR: %d\n", err);
		goto exit;
	}

	i = 0;
	/* print all keys / values */
	while (i < st.nkeys) {
		printf("%#08x, %#08x, %s, %s, %#08x\n",
			st.entry[i].key_hash,
			st.entry[i].value_hash,
			st.entry[i].key,
			st.entry[i].value,
			st.entry[i].section_hash
		);
		i++;
	}

	/* test setting a new value */
	cfg_value_set(&st, "key1", "\tvalue1 value1");

	/* print some values */
	puts("");
	printf("find value by key (key1): %s\n", cfg_value_get(&st, "key1"));
	printf("find value by key (key=8): %s\n", cfg_value_get(&st, "key=8"));
	printf("find key index (key3): %d\n", cfg_key_get_index(&st, "key3"));
	printf("find key by value (value3): %s\n", cfg_key_get(&st, "value3"));
	printf("get int (key5): %ld\n", cfg_value_get_long(&st, "key5", 10));
	printf("get float (key6): %f\n", cfg_value_get_double(&st, "key6"));
	printf("get hex (key7): %#lx\n", cfg_value_get_ulong(&st, "key7", 16));
	printf("get hex (key=8): %#lx\n", cfg_value_get_ulong(&st, "key=8", 16));
	printf("nkeys: %d\n", st.nkeys);
	printf("nsections: %d\n", st.nsections);

	/* test a section */
	puts("");
	entry = cfg_section_entry_get(&st, "section1", "key6");
	printf("test entry from section: %f\n", (entry) ? cfg_get_double(entry->value) : -1.0);
	entry = cfg_section_entry_get(&st, "section1", "key9");
	printf("test entry from section: %s\n", (entry) ? entry->value : "not found");
	puts("");

	/* dump the cache */
	puts("* cache");
	i = 0;
	while (i < st.cache_size) {
		if (st.cache[i]) {
			printf("%u, %08x, %08x, %s\n",
				st.cache[i]->index,
				st.cache[i]->key_hash,
				st.cache[i]->section_hash,
				st.cache[i]->value
			);
		} else {
			printf("empty cache pointer at index %d", 0);
		}
		i++;
	}

	exit:
	puts("");
	puts("* free");
	cfg_free(&st);
	puts("* end");
	return 0;
}
