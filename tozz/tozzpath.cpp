#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotozz.h>
#include <tozzpath.hpp>
#include <tozzstyle.hpp>
#include <tozzsvg.hpp>
#include <nemomisc.h>

struct tozzpath *nemotozz_path_create(void)
{
	struct tozzpath *path;

	path = new tozzpath;
	path->path = new SkPath;
	path->measure = NULL;

	return path;
}

void nemotozz_path_destroy(struct tozzpath *path)
{
	if (path->measure != NULL)
		delete path->measure;

	delete path->path;
	delete path;
}

void nemotozz_path_clear(struct tozzpath *path)
{
	path->path->reset();
}

void nemotozz_path_moveto(struct tozzpath *path, float x, float y)
{
	path->path->moveTo(x, y);
}

void nemotozz_path_lineto(struct tozzpath *path, float x, float y)
{
	path->path->lineTo(x, y);
}

void nemotozz_path_cubicto(struct tozzpath *path, float x0, float y0, float x1, float y1, float x2, float y2)
{
	path->path->cubicTo(x0, y0, x1, y1, x2, y2);
}

void nemotozz_path_arcto(struct tozzpath *path, float x, float y, float w, float h, float from, float to, int needs_moveto)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	path->path->arcTo(rect, from, to, needs_moveto == 0 ? false : true);
}

void nemotozz_path_close(struct tozzpath *path)
{
	path->path->close();
}

void nemotozz_path_arc(struct tozzpath *path, float x, float y, float w, float h, float from, float to)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	path->path->addArc(rect, from, to);
}

void nemotozz_path_text(struct tozzpath *path, struct tozzstyle *style, float x, float y, const char *text)
{
	SkPath rpath;
	style->paint->getTextPath(text, strlen(text), x, y - style->fontascent, &rpath);

	path->path->addPath(rpath);
}

void nemotozz_path_cmd(struct tozzpath *path, const char *cmd)
{
	SkPath rpath;
	SkParsePath::FromSVGString(cmd, &rpath);

	path->path->addPath(rpath);
}

void nemotozz_path_svg(struct tozzpath *path, const char *url, float x, float y, float w, float h)
{
	SkPath rpath;
	nemotozz_svg_load(url, x, y, w, h, &rpath);

	path->path->addPath(rpath);
}

void nemotozz_path_append(struct tozzpath *path, struct tozzpath *spath)
{
	path->path->addPath(*spath->path);
}

void nemotozz_path_translate(struct tozzpath *path, float tx, float ty)
{
	SkMatrix matrix;
	matrix.setIdentity();
	matrix.postTranslate(tx, ty);

	path->path->transform(matrix);
}

void nemotozz_path_scale(struct tozzpath *path, float sx, float sy)
{
	SkMatrix matrix;
	matrix.setIdentity();
	matrix.postScale(sx, sy);

	path->path->transform(matrix);
}

void nemotozz_path_rotate(struct tozzpath *path, float rz)
{
	SkMatrix matrix;
	matrix.setIdentity();
	matrix.postRotate(rz);

	path->path->transform(matrix);
}

void nemotozz_path_bounds(struct tozzpath *path, float *x, float *y, float *w, float *h)
{
	SkRect box = path->path->getBounds();

	*x = box.x();
	*y = box.y();
	*w = box.width();
	*h = box.height();
}

void nemotozz_path_measure(struct tozzpath *path)
{
	if (path->measure == NULL)
		path->measure = new SkPathMeasure;

	path->measure->setPath(path->path, false);
}

float nemotozz_path_length(struct tozzpath *path)
{
	return path->measure->getLength();
}

int nemotozz_path_position(struct tozzpath *path, float t, float *px, float *py, float *tx, float *ty)
{
	SkPoint point;
	SkVector tangent;
	bool r;

	r = path->measure->getPosTan(
			path->measure->getLength() * t,
			&point,
			&tangent);
	if (r == false)
		return -1;

	*px = point.x();
	*py = point.y();

	*tx = tangent.x();
	*ty = tangent.y();

	return 0;
}

void nemotozz_path_segment(struct tozzpath *path, float from, float to, struct tozzpath *spath)
{
	path->measure->getSegment(
			path->measure->getLength() * from,
			path->measure->getLength() * to,
			spath->path, true);
}

void nemotozz_path_dump(struct tozzpath *path, FILE *out)
{
	SkPath::Iter iter(*path->path, false);
	SkPath::Verb verb;
	SkPoint points[4];

	while ((verb = iter.next(points)) != SkPath::kDone_Verb) {
		switch (verb) {
			case SkPath::kMove_Verb:
				fprintf(out, "[MOVE] (%.2f, %.2f)\n",
						points[0].x(),
						points[0].y());
				break;

			case SkPath::kLine_Verb:
				fprintf(out, "[LINE] (%.2f, %.2f) (%.2f, %.2f)\n",
						points[0].x(),
						points[0].y(),
						points[1].x(),
						points[1].y());
				break;

			case SkPath::kQuad_Verb:
				fprintf(out, "[QUAD] (%.2f, %.2f) (%.2f, %.2f) (%.2f, %.2f)\n",
						points[0].x(),
						points[0].y(),
						points[1].x(),
						points[1].y(),
						points[2].x(),
						points[2].y());
				break;

			case SkPath::kConic_Verb:
				fprintf(out, "[CONIC] (%.2f, %.2f) (%.2f, %.2f) (%.2f, %.2f) weight(%f)\n",
						points[0].x(),
						points[0].y(),
						points[1].x(),
						points[1].y(),
						points[2].x(),
						points[2].y(),
						SkScalarToFloat(iter.conicWeight()));
				break;

			case SkPath::kCubic_Verb:
				fprintf(out, "[CUBIC] (%.2f, %.2f) (%.2f, %.2f) (%.2f, %.2f) (%.2f, %.2f)\n",
						points[0].x(),
						points[0].y(),
						points[1].x(),
						points[1].y(),
						points[2].x(),
						points[2].y(),
						points[3].x(),
						points[3].y());
				break;

			case SkPath::kClose_Verb:
				fprintf(out, "[CLOSE]\n");
				break;

			case SkPath::kDone_Verb:
				fprintf(out, "[DONE]\n");
				break;

			default:
				break;
		}
	}
}
