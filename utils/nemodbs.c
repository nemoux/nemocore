#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemodb.h>
#include <nemomisc.h>

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "collection",				required_argument,		NULL,		'c' },
		{ "file",							required_argument,		NULL,		'f' },
		{ 0 }
	};
	struct nemodb *db;
	struct dbiter *iter;
	struct nemoitem *item;
	struct itemone *one;
	char *cmd = NULL;
	char *collection = NULL;
	char *file = NULL;
	int opt;

	while (opt = getopt_long(argc, argv, "c:f:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'c':
				collection = strdup(optarg);
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

	if (cmd == NULL || collection == NULL)
		return -1;

	db = nemodb_create("mongodb://127.0.0.1");
	nemodb_use_collection(db, "nemodb", collection);

	if (strcmp(cmd, "drop") == 0) {
		nemodb_drop_collection(db);
	} else if (strcmp(cmd, "import") == 0) {
		item = nemoitem_create();
		nemoitem_load_textfile(item, file, ' ');

		nemoitem_for_each(one, item) {
			nemodb_insert_many(db, one);
		}

		nemoitem_destroy(item);
	} else if (strcmp(cmd, "export") == 0) {
		item = nemoitem_create();

		iter = nemodb_query_iter_all(db);

		while ((one = nemodb_iter_next(iter)) != NULL) {
			nemoitem_attach_one(item, one);
		}

		nemoitem_save_textfile(item, file, ' ');
		nemoitem_destroy(item);
	}

	nemodb_destroy(db);

	return 0;
}
