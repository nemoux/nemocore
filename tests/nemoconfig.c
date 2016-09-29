#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <json.h>

#include <nemokeys.h>
#include <nemomisc.h>

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "config",				required_argument,		NULL,		'c' },
		{ "namespace",		required_argument,		NULL,		'n' },
		{ "attr",					required_argument,		NULL,		'a' },
		{ "value",				required_argument,		NULL,		'v' },
		{ 0 }
	};
	struct nemokeys *keys;
	struct keysiter *iter;
	struct json_object *jobj;
	struct json_object *robj;
	char *config = NULL;
	char *cmd = NULL;
	char *namespace = NULL;
	char *contents = NULL;
	char *attr = NULL;
	char *value = NULL;
	int needs_destroy = 0;
	int opt;
	int i;

	while (opt = getopt_long(argc, argv, "c:n:a:v:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'c':
				config = strdup(optarg);
				break;

			case 'n':
				namespace = strdup(optarg);
				break;

			case 'a':
				attr = strdup(optarg);
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

	if (cmd == NULL || config == NULL)
		return -1;

	keys = nemokeys_create(config);
	if (keys == NULL)
		return -1;

	if (strcmp(cmd, "dump") == 0) {
		iter = nemokeys_create_iterator(keys);
		if (iter != NULL && nemokeys_iterator_seek_to_first(iter) != 0) {
			do {
				fprintf(stderr, "[%s]\n", nemokeys_iterator_key_safe(iter));

				jobj = json_tokener_parse(nemokeys_iterator_value_safe(iter));

				json_object_object_foreach(jobj, k, v) {
					fprintf(stderr, "  %s: %s\n", k, json_object_get_string(v));
				}

				json_object_put(jobj);
			} while (nemokeys_iterator_next(iter) != 0);

			nemokeys_destroy_iterator(iter);
		}
	} else if (strcmp(cmd, "clear") == 0) {
		nemokeys_clear(keys);
	} else {
		contents = nemokeys_get_safe(keys, namespace);
		if (contents != NULL) {
			jobj = json_tokener_parse(contents);
		} else {
			jobj = json_object_new_object();
		}

		if (strcmp(cmd, "set") == 0) {
			if (attr != NULL && value != NULL)
				json_object_object_add(jobj, attr, json_object_new_string(value));
		} else if (strcmp(cmd, "get") == 0) {
			if (attr != NULL) {
				if (json_object_object_get_ex(jobj, attr, &robj) != 0)
					fprintf(stderr, "%s: %s\n", attr, json_object_get_string(robj));
			} else {
				json_object_object_foreach(jobj, k, v) {
					fprintf(stderr, "%s: %s\n", k, json_object_get_string(v));
				}
			}
		} else if (strcmp(cmd, "put") == 0) {
			if (attr != NULL) {
				json_object_object_del(jobj, attr);
			} else {
				needs_destroy = 1;
			}
		}

		if (needs_destroy != 0 || json_object_object_length(jobj) == 0)
			nemokeys_put(keys, namespace);
		else
			nemokeys_set(keys, namespace, json_object_to_json_string(jobj));
	}

	nemokeys_destroy(keys);

	return 0;
}
