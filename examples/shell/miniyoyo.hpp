#ifndef	__MINISHELL_YOYO_HPP__
#define	__MINISHELL_YOYO_HPP__

#include <skiaconfig.hpp>

#include <minishell.h>

#include <showhelper.h>

struct miniyoyo {
	int32_t width, height;

	double x0, y0;
	double x1, y1;
	double x2, y2;

	SkPath *path;
	SkBitmap *bitmap;

	void *userdata;
};

extern struct miniyoyo *minishell_yoyo_create(int32_t width, int32_t height, SkPath *path);
extern void minishell_yoyo_destroy(struct miniyoyo *yoyo);

extern void minishell_yoyo_prepare(struct miniyoyo *yoyo, double x, double y);
extern void minishell_yoyo_update(struct miniyoyo *yoyo, double x, double y, int32_t *minx, int32_t *miny, int32_t *maxx, int32_t *maxy);
extern void minishell_yoyo_finish(struct miniyoyo *yoyo);

extern SkColor minishell_yoyo_get_pixel(struct miniyoyo *yoyo, int x, int y);

static inline void minishell_yoyo_set_userdata(struct miniyoyo *yoyo, void *userdata)
{
	yoyo->userdata = userdata;
}

static inline void *minishell_yoyo_get_userdata(struct miniyoyo *yoyo)
{
	return yoyo->userdata;
}

#endif
