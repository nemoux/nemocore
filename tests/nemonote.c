#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemonote.h>
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
	struct nemonote *note;
	struct noteobject *nobj;
	struct noteiter *niter;
	struct jsoniter *jiter;
	struct nemotoken *token;
	char *config = NULL;
	char *cmd = NULL;
	char *ns = NULL;
	char *attr = NULL;
	char *value = NULL;
	char *stream = NULL;
	char *file = NULL;
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
				ns = strdup(optarg);
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

	note = nemonote_create(config);
	if (note == NULL)
		return -1;

	if (strcmp(cmd, "dump") == 0) {
		niter = nemonote_create_iterator(note);
		if (niter != NULL) {
			do {
				fprintf(stderr, "[%s]\n", nemonote_iterator_key(niter));

				jiter = nemonote_json_iterator_create(nemonote_iterator_value(niter));
				if (jiter != NULL) {
					do {
						fprintf(stderr, "  %s: %s\n",
								nemonote_json_iterator_key(jiter),
								nemonote_json_iterator_value(jiter));
					} while (nemonote_json_iterator_next(jiter) != 0);

					nemonote_json_iterator_destroy(jiter);
				}
			} while (nemonote_iterator_next(niter) != 0);

			nemonote_destroy_iterator(niter);
		}
	} else if (strcmp(cmd, "clear") == 0) {
		nemonote_clear(note);
	} else if (strcmp(cmd, "load") == 0) {
		if (stream != NULL) {
			token = nemotoken_create(stream, strlen(stream));
			nemotoken_divide(token, ' ');
			nemotoken_update(token);

			nobj = nemonote_object_ref(note, nemotoken_get_token(token, 0));

			for (i = 1; i < nemotoken_get_token_count(token); i += 2) {
				nemonote_object_set(note, nobj,
						nemotoken_get_token(token, i + 0),
						nemotoken_get_token(token, i + 1));
			}

			nemonote_object_unref(note, nobj);

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

				nobj = nemonote_object_ref(note, nemotoken_get_token(token, 0));

				for (i = 1; i < nemotoken_get_token_count(token); i += 2) {
					nemonote_object_set(note, nobj,
							nemotoken_get_token(token, i + 0),
							nemotoken_get_token(token, i + 1));
				}

				nemonote_object_unref(note, nobj);

				nemotoken_destroy(token);
			}

			fclose(fp);
		}
	} else {
		if (strcmp(cmd, "set") == 0) {
			if (attr != NULL && value != NULL)
				nemonote_set_attr(note, ns, attr, value);
		} else if (strcmp(cmd, "get") == 0) {
			if (attr != NULL) {
				value = nemonote_get_attr(note, ns, attr);
				if (value != NULL)
					fprintf(stderr, "%s: %s\n", attr, value);
			} else {
				jiter = nemonote_json_iterator_create(nemonote_get(note, ns));
				if (jiter != NULL) {
					do {
						fprintf(stderr, "%s: %s\n",
								nemonote_json_iterator_key(jiter),
								nemonote_json_iterator_value(jiter));
					} while (nemonote_json_iterator_next(jiter) != 0);

					nemonote_json_iterator_destroy(jiter);
				}
			}
		} else if (strcmp(cmd, "put") == 0) {
			if (attr != NULL)
				nemonote_put_attr(note, ns, attr);
			else
				nemonote_put(note, ns);
		}
	}

	nemonote_destroy(note);

	return 0;
}
