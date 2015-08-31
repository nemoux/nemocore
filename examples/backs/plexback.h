#ifndef	__PLEXBACK_H__
#define	__PLEXBACK_H__

#include <nemoconfig.h>

#include <voronoihelper.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#define	PLEXBACK_VORONOI_ENABLE	(0)

struct plexback {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	int32_t width, height;

#if PLEXBACK_VORONOI_ENABLE
	std::vector<Point> points;
	std::vector<Segment> segments;
#else
	std::vector<p2t::Point *> spoints;
	std::vector<p2t::Point *> dpoints;
#endif

	uint32_t msecs;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
