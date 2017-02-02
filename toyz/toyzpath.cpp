#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotoyz.h>
#include <toyzpath.hpp>
#include <nemomisc.h>

struct toyzpath *nemotoyz_path_create(void)
{
	struct toyzpath *path;

	path = new toyzpath;
	path->path = new SkPath;

	return path;
}

void nemotoyz_path_destroy(struct toyzpath *path)
{
	delete path->path;
	delete path;
}

void nemotoyz_path_clear(struct toyzpath *path)
{
	path->path->reset();
}

void nemotoyz_path_moveto(struct toyzpath *path, float x, float y)
{
	path->path->moveTo(x, y);
}

void nemotoyz_path_lineto(struct toyzpath *path, float x, float y)
{
	path->path->lineTo(x, y);
}

void nemotoyz_path_cubicto(struct toyzpath *path, float x0, float y0, float x1, float y1, float x2, float y2)
{
	path->path->cubicTo(x0, y0, x1, y1, x2, y2);
}

void nemotoyz_path_arcto(struct toyzpath *path, float x, float y, float w, float h, float from, float to, int needs_moveto)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	path->path->arcTo(rect, from, to, needs_moveto == 0 ? false : true);
}

void nemotoyz_path_close(struct toyzpath *path)
{
	path->path->close();
}

void nemotoyz_path_cmd(struct toyzpath *path, const char *cmd)
{
	SkPath rpath;
	SkParsePath::FromSVGString(cmd, &rpath);

	path->path->addPath(rpath);
}

void nemotoyz_path_arc(struct toyzpath *path, float x, float y, float w, float h, float from, float to)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	path->path->addArc(rect, from, to);
}

void nemotoyz_path_append(struct toyzpath *path, struct toyzpath *spath)
{
	path->path->addPath(*spath->path);
}

void nemotoyz_path_translate(struct toyzpath *path, float tx, float ty)
{
	SkMatrix matrix;
	matrix.setIdentity();
	matrix.postTranslate(tx, ty);

	path->path->transform(matrix);
}

void nemotoyz_path_scale(struct toyzpath *path, float sx, float sy)
{
	SkMatrix matrix;
	matrix.setIdentity();
	matrix.postScale(sx, sy);

	path->path->transform(matrix);
}

void nemotoyz_path_rotate(struct toyzpath *path, float rz)
{
	SkMatrix matrix;
	matrix.setIdentity();
	matrix.postRotate(rz);

	path->path->transform(matrix);
}
