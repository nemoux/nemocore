#ifndef __NEMO_FS_H__
#define __NEMO_FS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct fsdir {
	char **files;
	int nfiles;

	char *path;
};

extern struct fsdir *nemofs_dir_create(const char *path);
extern struct fsdir *nemofs_dir_create_for_directories(const char *path);
extern struct fsdir *nemofs_dir_create_for_files(const char *path);
extern struct fsdir *nemofs_dir_create_for_extensions(const char *path, const char *extensions);
extern void nemofs_dir_destroy(struct fsdir *dir);

static inline int nemofs_dir_get_filecount(struct fsdir *dir)
{
	return dir->nfiles;
}

static inline const char *nemofs_dir_get_filename(struct fsdir *dir, int index)
{
	return dir->files[index];
}

static inline void nemofs_dir_get_filepath(struct fsdir *dir, int index, char *filepath)
{
	strcpy(filepath, dir->path);
	strcat(filepath, "/");
	strcat(filepath, dir->files[index]);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
