#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <json.h>

#include <nemokeys.h>
#include <nemotoken.h>
#include <nemomisc.h>

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "config",				required_argument,		NULL,		'c' },
		{ "namespace",		required_argument,		NULL,		'n' },
		{ "attr",					required_argument,		NULL,		'a' },
		{ "value",				required_argument,		NULL,		'v' },
		{ "stream",				required_argument,		NULL,		's' },
		{ "file",					required_argument,		NULL,		'f' },
		{ 0 }
	};
	struct nemokeys *keys;
	struct keysiter *iter;
	struct nemotoken *token;
	struct json_object *jobj;
	struct json_object *robj;
	struct json_object_iterator jiter;
	struct json_object_iterator jiter0;
	char *config = NULL;
	char *cmd = NULL;
	char *namespace = NULL;
	char *contents = NULL;
	char *attr = NULL;
	char *value = NULL;
	char *stream = NULL;
	char *file = NULL;
	int needs_destroy = 0;
	int opt;
	int i;

	while (opt = getopt_long(argc, argv, "c:n:a:v:s:f:", options, NULL)) {
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

			case 's':
				stream = strdup(optarg);
				break;

			case 'f':
				file = strdup(optarg);
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

				jiter = json_object_iter_begin(jobj);
				jiter0 = json_object_iter_end(jobj);

				while (json_object_iter_equal(&jiter, &jiter0) == 0) {
					fprintf(stderr, "  %s: %s\n",
							json_object_iter_peek_name(&jiter),
							json_object_get_string(json_object_iter_peek_value(&jiter)));

					json_object_iter_next(&jiter);
				}

				json_object_put(jobj);
			} while (nemokeys_iterator_next(iter) != 0);

			nemokeys_destroy_iterator(iter);
		}
	} else if (strcmp(cmd, "clear") == 0) {
		nemokeys_clear(keys);
	} else if (strcmp(cmd, "load") == 0) {
		if (stream != NULL) {
			token = nemotoken_create(stream, strlen(stream));
			nemotoken_divide(token, ' ');
			nemotoken_update(token);

			jobj = json_object_new_object();

			for (i = 1; i < nemotoken_get_token_count(token); i += 2) {
				json_object_object_add(jobj,
						nemotoken_get_token(token, i + 0),
						json_object_new_string(nemotoken_get_token(token, i + 1)));
			}

			nemokeys_set(keys, nemotoken_get_token(token, 0), json_object_to_json_string(jobj));

			json_object_put(jobj);

			nemotoken_destroy(token);
		} else if (file != NULL) {
			FILE *fp;
			char buffer[1024];

			fp = fopen(file, "r");
			if (fp == NULL)
				return -1;

			while (fgets(buffer, sizeof(buffer), fp) != NULL) {
				int length = strlen(buffer);

				if (buffer[0] == '/' && buffer[1] == '/')
					continue;
				if (buffer[0] != '/')
					continue;
				if (buffer[length - 1] == '\n')
					buffer[length - 1] = '\0';

				token = nemotoken_create(buffer, strlen(buffer));
				nemotoken_divide(token, ' ');
				nemotoken_update(token);

				jobj = json_object_new_object();

				for (i = 1; i < nemotoken_get_token_count(token); i += 2) {
					json_object_object_add(jobj,
							nemotoken_get_token(token, i + 0),
							json_object_new_string(nemotoken_get_token(token, i + 1)));
				}

				nemokeys_set(keys, nemotoken_get_token(token, 0), json_object_to_json_string(jobj));

				json_object_put(jobj);

				nemotoken_destroy(token);
			}

			fclose(fp);
		}
	} else {
		contents = nemokeys_get_safe(keys, namespace);
		if (contents != NULL) {
			jobj = json_tokener_parse(contents);

			free(contents);
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
				jiter = json_object_iter_begin(jobj);
				jiter0 = json_object_iter_end(jobj);

				while (json_object_iter_equal(&jiter, &jiter0) == 0) {
					fprintf(stderr, "%s: %s\n",
							json_object_iter_peek_name(&jiter),
							json_object_get_string(json_object_iter_peek_value(&jiter)));

					json_object_iter_next(&jiter);
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

		json_object_put(jobj);
	}

	nemokeys_destroy(keys);

	return 0;
}
