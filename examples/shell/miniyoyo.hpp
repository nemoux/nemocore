#ifndef	__MINISHELL_YOYO_HPP__
#define	__MINISHELL_YOYO_HPP__

#include <skiaconfig.hpp>

#include <minishell.h>

#include <showhelper.h>

struct miniyoyo {
	struct showone *path;

	int32_t width, height;

	double x0, y0;
	double x1, y1;
	double x2, y2;

	int32_t minx, miny, maxx, maxy;

	SkBitmap *bitmap;
};

extern struct miniyoyo *minishell_yoyo_create(int32_t width, int32_t height, struct showone *path);
extern void minishell_yoyo_destroy(struct miniyoyo *yoyo);

extern void minishell_yoyo_dispatch_down_event(struct miniyoyo *yoyo, double x, double y);
extern void minishell_yoyo_dispatch_motion_event(struct miniyoyo *yoyo, double x, double y);
extern void minishell_yoyo_dispatch_up_event(struct miniyoyo *yoyo);

extern SkColor minishell_yoyo_get_pixel(struct miniyoyo *yoyo, int x, int y);

#endif
