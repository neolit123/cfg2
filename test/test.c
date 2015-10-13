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
#include <time.h>
#include "cfg2.h"

#define VERBOSE        2
#define PRINT_TESTS    1
#define PRINT_CACHE    1

/*
 * test parsing a file or a buffer directly. when calling a parsing method
 * the memory pointed by the cfg_t will be released automatically.
 * when you are done with the library make sure to call cfg_free().
 */
int main(void)
{
	cfg_uint32 i;
	cfg_error_t err;
	cfg_t st;
	cfg_entry_t *entry;
	char buf[] =
"key1=\\tvalue1\n" \
"key2=value2\n" \
"key3=value3\n" \
"key4=value4\n";
	char file[] = "test.cfg";

	clock_t begin, end;
	double time_spent;

	(void)entry;

	puts("[cfg2 test]");
	puts("* init");
	/* init the structure with cache buffer size of 4. this means that 4 unique
	 * (and fast) entries will be cached at all times. */
	err = cfg_init(&st);
	st.cache_size = 4;
	st.verbose = VERBOSE;
	if (err > 0) {
		printf("cfg_init() ERROR: %d\n", err);
		goto exit;
	}
	printf("* cache size: %d\n", st.cache_size);
	puts("* parse");
	err = cfg_parse_buffer(&st, buf, strlen(buf));

	begin = clock();
	err = cfg_parse_file(&st, file);
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("time spent parsing %s: %.4f sec\n", file, time_spent);
	printf("nkeys: %d\n", st.nkeys);
	printf("nsections: %d\n", st.nsections);

	if (err > 0) {
		printf("cfg_parse_file() ERROR: %d\n", err);
		goto exit;
	}
#if (PRINT_TESTS == 1)
	printf("press any key to continue...");
	getchar();

	puts("");
	puts("* actions");

	/* print all keys / values */
	for (i = 0; i < st.nkeys; i++) {
		printf("%#08x, %s, %s, %#08x\n",
			st.entry[i].key_hash,
			st.entry[i].key,
			st.entry[i].value,
			st.entry[i].section_hash
		);
	}

	/* list sections */
	puts("\nlist sections:");
	for (i = 0; i < st.nsections; i++)
		printf("%d, %s\n", i, st.section[i]);

	/* test setting a new value */
	cfg_value_set(&st, "key1", "\tvalue1 value1");

	/* print some values */
	puts("");
	printf("find value by key (key1): %s\n", cfg_value_get(&st, "key1"));
	printf("find value by key (key=8): %s\n", cfg_value_get(&st, "key=8"));
	printf("find key index (key3): %d\n", cfg_key_get_index(&st, "key3"));
	printf("get int (key5): %ld\n", cfg_value_get_long(&st, "key5", 10));
	printf("get float (key6): %f\n", cfg_value_get_double(&st, "key6"));
	printf("get hex (key[7]): %#lx\n", cfg_value_get_ulong(&st, "key[7]", 16));
	printf("get hex (key=8): %#lx\n", cfg_value_get_ulong(&st, "key=8", 16));

	/* test a section */
	puts("");
	entry = cfg_section_entry_get(&st, "section1", "key6");
	printf("test entry from section: %f\n", (entry) ? cfg_get_double(entry->value) : -1.0);
	entry = cfg_section_entry_get(&st, "section1", "key9");
	printf("test entry from section: %s\n", (entry) ? entry->value : "not found");

	/* test hex to char */
	puts("");
	entry = cfg_section_entry_get(&st, "section2", "key13");
	if (entry)
		cfg_entry_value_hex_to_char(&st, entry);
	entry = cfg_section_entry_get(&st, "section2", "key14");
	if (entry)
		cfg_entry_value_hex_to_char(&st, entry);
#endif

#if (PRINT_CACHE == 1)
	/* dump the cache */
	puts("");
	puts("* cache");
	for (i = 0; i < st.cache_size; i++) {
		if (st.cache[i]) {
			printf("%u, %08x, %08x, %s\n",
				st.cache[i]->index,
				st.cache[i]->key_hash,
				st.cache[i]->section_hash,
				st.cache[i]->value
			);
		} else {
			printf("empty cache pointer at index %d\n", 0);
		}
	}
#endif

exit:
	puts("");
	puts("* free");
	cfg_free(&st);
	puts("* end");
	return 0;
}
