/*
 * cfg2 0.1
 * a simplistic configuration parser for INI like syntax in ANSI C
 *
 * author: lubomir i. ivanov (neolit123 at gmail)
 * this code is released in the public domain without warranty of any kind
 *
 * test.c:
 *	test file for the api
 */

#include <stdio.h>
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
	char buf[] =
"key1=\\tvalue1\n" \
"key2=value2\n" \
"key3=value3\n" \
"key4=value4\n\0";

	puts("[cfg2 test]");
	puts("* init");
	err = cfg_init(&st); /* make sure we init the structure */
	if (err > 0) {
		printf("cfg_init() ERROR: %d\n", err);
		goto exit;
	}
	err = cfg_cache_size_set(&st, 8);
	if (err > 0) {
		printf("cfg_cache_size_set() ERROR: %d\n", err);
		goto exit;
	}
	printf("* cache size: %d\n", st.cache_size);
	puts("* parse");
	err = cfg_parse_buffer(&st, buf, strlen(buf));
	err = cfg_parse_file(&st, "test.cfg");
	puts("* actions");
	if (err > 0) {
		printf("cfg_parse_file() ERROR: %d\n", err);
		goto exit;
	}

	i = 0;
	/* print all keys / values */
	while (i < st.nkeys) {
		printf("%#08x, %#08x, %s, %s\n", st.keys_hash[i], st.values_hash[i], st.keys[i], st.values[i]);
		i++;
	}

	/* print some values */
	printf("find value by key (key1): %s\n", cfg_value_get(&st, "key1"));
	printf("find key index (key3): %d\n", cfg_key_get_index(&st, "key3"));
	printf("find key by value (value3): %s\n", cfg_key_get(&st, "value3"));
	printf("get int (key5): %ld\n", cfg_value_get_long(&st, "key5", 10));
	printf("get hex (key7): %#lx\n", cfg_value_get_ulong(&st, "key7", 16));
	printf("get float (key6): %f\n", cfg_value_get_double(&st, "key6"));
	printf("nkeys: %d\n", st.nkeys);

	/* dump the cache */
	puts("* cache");
	i = 0;
	while (i < st.cache_size) {
		printf("cache item: %d, index: %d, value: %s\n", i, st.cache_keys_index[i], st.values[st.cache_keys_index[i]]);
		i++;
	}

	exit:
	puts("* free");
	cfg_free(&st);
	puts("* end");
  return 0;
}
