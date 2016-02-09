#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#define	SK_RELEASE			1
#define	SK_CPU_LENDIAN	1
#define SK_R32_SHIFT		16
#define SK_G32_SHIFT		8
#define SK_B32_SHIFT		0
#define SK_A32_SHIFT		24

#include <SkTypes.h>
#include <SkCanvas.h>
#include <SkGraphics.h>
#include <SkImageInfo.h>
#include <SkImageEncoder.h>
#include <SkImageDecoder.h>
#include <SkBitmapDevice.h>
#include <SkBitmap.h>
#include <SkStream.h>
#include <SkString.h>
#include <SkMatrix.h>
#include <SkRegion.h>
#include <SkParsePath.h>
#include <SkTypeface.h>

#include <SkBlurMask.h>
#include <SkBlurMaskFilter.h>
#include <SkEmbossMaskFilter.h>

#include <SkGradientShader.h>

#include <SkGeometry.h>

#include <poly2tri.h>

#include <nemotool.h>
#include <nemoegl.h>
#include <pixmanhelper.h>
#include <talehelper.h>
#include <voronoihelper.h>
#include <nemomisc.h>

#include <voronoihelper.h>

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

static void nemoback_plex_render_one(struct plexback *plex, pixman_image_t *image, double t)
{
	int32_t width = pixman_image_get_width(image);
	int32_t height = pixman_image_get_height(image);
	int i;

	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(
			pixman_image_get_data(image));

	SkBitmapDevice device(bitmap);
	SkCanvas canvas(&device);

	canvas.clear(SK_ColorTRANSPARENT);

	SkMaskFilter *filter = SkBlurMaskFilter::Create(
			kSolid_SkBlurStyle,
			SkBlurMask::ConvertRadiusToSigma(5.0f),
			SkBlurMaskFilter::kHighQuality_BlurFlag);

	SkPaint stroke;
	stroke.setAntiAlias(true);
	stroke.setStyle(SkPaint::kStroke_Style);
	stroke.setStrokeWidth(1.0f);
	stroke.setColor(SkColorSetARGB(63.0f, 0.0f, 255.0f, 255.0f));
	stroke.setMaskFilter(filter);

	SkPaint fill;
	fill.setAntiAlias(true);
	fill.setStyle(SkPaint::kFill_Style);
	fill.setColor(SkColorSetARGB(128.0f, 0.0f, 255.0f, 255.0f));
	fill.setMaskFilter(filter);

	SkPath path;

#if PLEXBACK_VORONOI_ENABLE
	plex->points[0] = Point(width * t, height * t);

	voronoi_diagram<double> vd;
	construct_voronoi(
			plex->points.begin(), plex->points.end(),
			plex->segments.begin(), plex->segments.end(),
			&vd);

	for (voronoi_diagram<double>::const_edge_iterator it = vd.edges().begin(); it != vd.edges().end(); it++) {
		if (it->vertex0() != NULL && it->vertex1() != NULL) {
			path.moveTo(it->vertex0()->x(), it->vertex0()->y());
			path.lineTo(it->vertex1()->x(), it->vertex1()->y());
		}
	}

	for (i = 0; i < plex->points.size(); i++) {
		Point &p = plex->points[i];

		canvas.drawCircle(x(p), y(p), 3.0f, fill);
	}
#else
	std::vector<p2t::Point *> polyline;
	polyline.push_back(new p2t::Point(0.0f, 0.0f));
	polyline.push_back(new p2t::Point(0.0f, height));
	polyline.push_back(new p2t::Point(width, height));
	polyline.push_back(new p2t::Point(width, 0.0f));

	p2t::CDT *cdt = new p2t::CDT(polyline);

	for (i = 0; i < plex->spoints.size(); i++) {
		cdt->AddPoint(new p2t::Point(
					(plex->dpoints[i]->x - plex->spoints[i]->x) * t + plex->spoints[i]->x,
					(plex->dpoints[i]->y - plex->spoints[i]->y) * t + plex->spoints[i]->y));
	}

	cdt->Triangulate();

	std::vector<p2t::Triangle *> triangles = cdt->GetTriangles();
	std::list<p2t::Triangle *> maps = cdt->GetMap();

#if 0
	for (i = 0; i < triangles.size(); i++) {
		p2t::Triangle *t = triangles[i];
		p2t::Point *a = t->GetPoint(0);
		p2t::Point *b = t->GetPoint(1);
		p2t::Point *c = t->GetPoint(2);

		SkPoint points[3] = {
			SkPoint::Make(a->x, a->y),
			SkPoint::Make(b->x, b->y),
			SkPoint::Make(c->x, c->y)
		};

		path.addPoly(points, 3, true);
	}
#else
	std::list<p2t::Triangle *>::iterator it;

	for (it = maps.begin(); it != maps.end(); it++) {
		p2t::Triangle *t = *it;
		p2t::Point *a = t->GetPoint(0);
		p2t::Point *b = t->GetPoint(1);
		p2t::Point *c = t->GetPoint(2);

		path.moveTo(a->x, a->y);
		path.lineTo(b->x, b->y);
		path.lineTo(c->x, c->y);
	}

	for (it = maps.begin(); it != maps.end(); it++) {
		p2t::Triangle *t = *it;
		p2t::Point *a = t->GetPoint(0);
		p2t::Point *b = t->GetPoint(1);
		p2t::Point *c = t->GetPoint(2);

		canvas.drawCircle(a->x, a->y, 5.0f, fill);
		canvas.drawCircle(b->x, b->y, 5.0f, fill);
		canvas.drawCircle(c->x, c->y, 5.0f, fill);
	}
#endif
#endif

	canvas.drawPath(path, stroke);
}

static void nemoback_plex_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct plexback *plex = (struct plexback *)nemotale_get_userdata(tale);
	struct talenode *node = plex->node;
	uint32_t msecs;

	if (secs == 0 && nsecs == 0) {
		plex->msecs = msecs = time_current_msecs();

		nemocanvas_dispatch_feedback(canvas);
	} else {
		msecs = time_current_msecs();

		nemocanvas_dispatch_feedback(canvas);
	}

	nemoback_plex_render_one(plex, nemotale_node_get_pixman(node), (double)(msecs - plex->msecs) / 100000.0f);

	nemotale_node_damage_all(node);

	nemotale_composite_egl(tale, NULL);
}

static void nemoback_plex_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",			required_argument,			NULL,		'w' },
		{ "height",			required_argument,			NULL,		'h' },
		{ 0 }
	};
	struct plexback *plex;
	struct nemotool *tool;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	struct talenode *node;
	int32_t width = 1920;
	int32_t height = 1080;
	int opt;
	int i;

	while (opt = getopt_long(argc, argv, "w:h:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	plex = (struct plexback *)malloc(sizeof(struct plexback));
	if (plex == NULL)
		return -1;
	memset(plex, 0, sizeof(struct plexback));

	plex->width = width;
	plex->height = height;

#if PLEXBACK_VORONOI_ENABLE
	for (i = 0; i < 500; i++) {
		plex->points.push_back(Point(
					random_get_double(0, width),
					random_get_double(0, height)));
	}
#else
	for (i = 0; i < 500; i++) {
		plex->spoints.push_back(new p2t::Point(
					random_get_double(0, width),
					random_get_double(0, height)));
	}

	for (i = 0; i < 500; i++) {
		plex->dpoints.push_back(new p2t::Point(
					random_get_double(0, width),
					random_get_double(0, height)));
	}
#endif

	plex->tool = tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	plex->egl = egl = nemotool_create_egl(tool);

	plex->eglcanvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_opaque(NTEGL_CANVAS(canvas), 0, 0, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_layer(NTEGL_CANVAS(canvas), NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	nemocanvas_set_dispatch_frame(NTEGL_CANVAS(canvas), nemoback_plex_dispatch_canvas_frame);
	nemocanvas_unset_sound(NTEGL_CANVAS(canvas));

	plex->canvas = NTEGL_CANVAS(canvas);

	plex->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(canvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), nemoback_plex_dispatch_tale_event);
	nemotale_set_userdata(tale, plex);

	plex->node = node = nemotale_node_create_pixman(width, height);
	nemotale_node_set_id(node, 1);
	nemotale_node_opaque(node, 0, 0, width, height);
	nemotale_attach_node(tale, node);

	nemocanvas_dispatch_frame(NTEGL_CANVAS(canvas));

	nemotool_run(tool);

	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	free(plex);

	return 0;
}
