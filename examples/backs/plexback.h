#ifndef	__PLEXBACK_H__
#define	__PLEXBACK_H__

#include <nemoconfig.h>

#include <voronoihelper.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct plexback {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	int32_t width, height;

#if	0
	std::vector<p2t::Point *> spoints;
	std::vector<p2t::Point *> dpoints;
#else
	std::vector<Point> points;
	std::vector<Segment> segments;
#endif

	uint32_t msecs;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
