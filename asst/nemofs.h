#ifndef __NEMO_FS_H__
#define __NEMO_FS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct fsdir {
	char **filenames;
	char **filepaths;
	int nfiles;
	int mfiles;

	char *path;
};

extern struct fsdir *nemofs_dir_create(const char *path, int minimum_files);
extern void nemofs_dir_destroy(struct fsdir *dir);

extern void nemofs_dir_clear(struct fsdir *dir);

extern int nemofs_dir_scan_directories(struct fsdir *dir);
extern int nemofs_dir_scan_files(struct fsdir *dir);
extern int nemofs_dir_scan_extension(struct fsdir *dir, const char *extension);

extern int nemofs_dir_insert_file(struct fsdir *dir, const char *filename);

static inline int nemofs_dir_get_filecount(struct fsdir *dir)
{
	return dir->nfiles;
}

static inline const char *nemofs_dir_get_filename(struct fsdir *dir, int index)
{
	return dir->filenames[index];
}

static inline const char *nemofs_dir_get_filepath(struct fsdir *dir, int index)
{
	return dir->filepaths[index];
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
