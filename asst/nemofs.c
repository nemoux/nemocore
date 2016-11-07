#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <dirent.h>

#include <nemofs.h>
#include <nemotoken.h>
#include <oshelper.h>
#include <nemomisc.h>

struct fsdir *nemofs_dir_create(const char *path)
{
	struct fsdir *dir;
	struct dirent **entries;
	const char *filename;
	int i, count;

	dir = (struct fsdir *)malloc(sizeof(struct fsdir));
	if (dir == NULL)
		return NULL;
	memset(dir, 0, sizeof(struct fsdir));

	dir->path = strdup(path);

	count = scandir(path, &entries, NULL, alphasort);

	dir->files = (char **)malloc(sizeof(char *) * count);

	for (i = 0; i < count; i++) {
		filename = entries[i]->d_name;

		dir->files[i] = strdup(filename);
	}

	dir->nfiles = count;

	free(entries);

	return dir;
}

struct fsdir *nemofs_dir_create_for_directories(const char *path)
{
	struct fsdir *dir;
	struct dirent **entries;
	const char *filename;
	char filepath[128];
	int idx = 0;
	int i, count;

	dir = (struct fsdir *)malloc(sizeof(struct fsdir));
	if (dir == NULL)
		return NULL;
	memset(dir, 0, sizeof(struct fsdir));

	dir->path = strdup(path);

	count = scandir(path, &entries, NULL, alphasort);

	dir->files = (char **)malloc(sizeof(char *) * count);

	for (i = 0; i < count; i++) {
		filename = entries[i]->d_name;

		strcpy(filepath, path);
		strcat(filepath, "/");
		strcat(filepath, filename);

		if (os_check_path_is_directory(filepath) != 0)
			dir->files[idx++] = strdup(filename);
	}

	dir->nfiles = idx;

	free(entries);

	return dir;
}

struct fsdir *nemofs_dir_create_for_files(const char *path)
{
	struct fsdir *dir;
	struct dirent **entries;
	const char *filename;
	char filepath[128];
	int idx = 0;
	int i, count;

	dir = (struct fsdir *)malloc(sizeof(struct fsdir));
	if (dir == NULL)
		return NULL;
	memset(dir, 0, sizeof(struct fsdir));

	dir->path = strdup(path);

	count = scandir(path, &entries, NULL, alphasort);

	dir->files = (char **)malloc(sizeof(char *) * count);

	for (i = 0; i < count; i++) {
		filename = entries[i]->d_name;

		strcpy(filepath, path);
		strcat(filepath, "/");
		strcat(filepath, filename);

		if (os_check_path_is_file(filepath) != 0)
			dir->files[idx++] = strdup(filename);
	}

	dir->nfiles = idx;

	free(entries);

	return dir;
}

struct fsdir *nemofs_dir_create_for_extensions(const char *path, const char *extensions)
{
	struct fsdir *dir;
	struct dirent **entries;
	struct nemotoken *exts;
	const char *filename;
	const char *fileext;
	char filepath[128];
	int idx = 0;
	int i, count;
	int e;

	dir = (struct fsdir *)malloc(sizeof(struct fsdir));
	if (dir == NULL)
		return NULL;
	memset(dir, 0, sizeof(struct fsdir));

	dir->path = strdup(path);

	exts = nemotoken_create(extensions, strlen(extensions));
	nemotoken_divide(exts, ';');
	nemotoken_update(exts);

	count = scandir(path, &entries, NULL, alphasort);

	dir->files = (char **)malloc(sizeof(char *) * count);

	for (i = 0; i < count; i++) {
		filename = entries[i]->d_name;

		strcpy(filepath, path);
		strcat(filepath, "/");
		strcat(filepath, filename);

		if (os_check_path_is_file(filepath) != 0) {
			fileext = os_get_file_extension(filename);
			if (fileext != NULL) {
				for (e = 0; e < nemotoken_get_token_count(exts); e++) {
					if (strcmp(fileext, nemotoken_get_token(exts, e)) == 0) {
						dir->files[idx++] = strdup(filename);
						break;
					}
				}
			}
		}
	}

	dir->nfiles = idx;

	free(entries);

	nemotoken_destroy(exts);

	return dir;
}

void nemofs_dir_destroy(struct fsdir *dir)
{
	int i;

	for (i = 0; i < dir->nfiles; i++) {
		free(dir->files[i]);
	}

	free(dir->files);
	free(dir->path);

	free(dir);
}
