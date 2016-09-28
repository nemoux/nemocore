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
			fprintf(stderr, "%s => %s\n", key, value);
			free(value);
		}
	} else if (strcmp(cmd, "dump") == 0) {
		iter = nemokeys_create_iterator(keys);
		if (iter != NULL && nemokeys_iterator_seek_to_first(iter) != 0) {
			do {
				fprintf(stderr, "%s => %s\n",
						nemokeys_iterator_key(iter),
						nemokeys_iterator_value(iter));
			} while (nemokeys_iterator_next(iter) != 0);

			nemokeys_destroy_iterator(iter);
		}
	}

	nemokeys_destroy(keys);

	return 0;
}
