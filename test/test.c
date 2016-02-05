/*
 * cfg2
 * a simplistic configuration parser for INI like syntax in C
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
#define PRINT_CACHE    0

/*
 * test parsing a file or a buffer directly. when calling a parsing method
 * the memory pointed by the cfg_t will be released automatically.
 * when you are done with the library make sure to call cfg_free().
 */
int main(void)
{
	cfg_uint32 i;
	cfg_status_t err;
	cfg_t st;
	cfg_entry_t *entry;
	char buf[] =
"key1=value1\n\n\n\n" \
"[s]\n" \
"key2=value2\n" \
"key3=value3\n" \
"key4=value4\n";
	char file[] = "test.cfg";
	cfg_char *write_buf, *ptr;
	cfg_uint32 write_len;

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
	err = cfg_parse_buffer(&st, buf, strlen(buf), CFG_TRUE);

	begin = clock();
	err = cfg_parse_file(&st, file);
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("time spent parsing %s: %.4f sec\n", file, time_spent);
	printf("nentries: %d\n", st.nentries);
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
	for (i = 0; i < st.nentries; i++) {
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
	puts("\nattempting to set value...");
	err = cfg_value_set(&st, "section1", "key1", "\tnew value1", CFG_FALSE);
	if (!err)
		printf("new value: %s\n", cfg_value_get(&st, "section1", "key1"));
	else
		printf("setting value error: %d\n", err);

	/* print some values */
	puts("");
	printf("find value by key (key1): %s\n", cfg_value_get(&st, "section1", "key1"));
	printf("find value by key (key=8): %s\n", cfg_value_get(&st, "section1", "key=8"));

	/* test a section */
	puts("");
	entry = cfg_entry_get(&st, "section1", "key6");
	printf("test entry from section: %f\n", (entry) ? cfg_value_to_double(entry->value) : -1.0);
	entry = cfg_entry_get(&st, "section1", "key9");
	printf("test entry from section: %s\n", (entry) ? entry->value : "not found");

	puts("");
	puts("test conversations:");
	printf("%u\n", cfg_value_to_bool("0"));

	/* test hex <-> char */
	puts("");
	entry = cfg_entry_get(&st, "section2", "key13");
	ptr = cfg_hex_to_char(&st, entry->value);
	puts(ptr);
	puts(cfg_char_to_hex(&st, ptr));
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

	puts("");
	printf("test delete: %d\n", cfg_section_delete(&st, "section2"));

	puts("");
	err = cfg_write_buffer(&st, &write_buf, &write_len);
	printf("write buf (%d):\n", write_len);
	if (write_buf) {
		puts(write_buf);
		free(write_buf);
	}

	printf("write_file() status: %d\n", cfg_write_file(&st, "out.cfg"));

exit:
	puts("");
	puts("* free");
	cfg_free(&st, CFG_FALSE);
	puts("* end");
	return 0;
}
