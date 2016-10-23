#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemokeys.h>
#include <nemomisc.h>

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "db",				required_argument,		NULL,		'd' },
		{ "key",			required_argument,		NULL,		'k' },
		{ "value",		required_argument,		NULL,		'v' },
		{ 0 }
	};
	struct nemokeys *keys;
	struct keysiter *iter;
	char *db = NULL;
	char *cmd = NULL;
	char *key = NULL;
	char *value = NULL;
	int opt;

	while (opt = getopt_long(argc, argv, "d:k:v:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'd':
				db = strdup(optarg);
				break;

			case 'k':
				key = strdup(optarg);
				break;

			case 'v':
				value = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (optind < argc)
		cmd = strdup(argv[optind]);

	if (cmd == NULL || db == NULL)
		return -1;

	keys = nemokeys_create(db);
	if (keys == NULL)
		return -1;

	if (strcmp(cmd, "set") == 0) {
		nemokeys_set(keys, key, value);
	} else if (strcmp(cmd, "get") == 0) {
		value = nemokeys_get(keys, key);
		if (value != NULL) {
			fprintf(stderr, "%s: %s\n", key, value);
			free(value);
		}
	} else if (strcmp(cmd, "put") == 0) {
		nemokeys_put(keys, key);
	} else if (strcmp(cmd, "dump") == 0) {
		for (iter = nemokeys_create_iterator(keys); nemokeys_iterator_valid(iter) != 0; nemokeys_iterator_next(iter)) {
			fprintf(stderr, "%s: %s\n",
					nemokeys_iterator_key_safe(iter),
					nemokeys_iterator_value_safe(iter));
		}

		nemokeys_destroy_iterator(iter);
	}

	nemokeys_destroy(keys);

	return 0;
}