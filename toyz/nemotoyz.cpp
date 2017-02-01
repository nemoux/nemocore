#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotoyz.hpp>
#include <toyzstyle.hpp>
#include <toyzpath.hpp>
#include <toyzmatrix.hpp>
#include <toyzregion.hpp>
#include <nemomisc.h>

struct nemotoyz *nemotoyz_create(void)
{
	struct nemotoyz *toyz;

	toyz = new nemotoyz;
	toyz->bitmap = NULL;
	toyz->device = NULL;
	toyz->canvas = NULL;

	return toyz;
}

void nemotoyz_destroy(struct nemotoyz *toyz)
{
	if (toyz->bitmap != NULL)
		delete toyz->bitmap;
	if (toyz->device != NULL)
		delete toyz->device;
	if (toyz->canvas != NULL)
		delete toyz->canvas;

	delete toyz;
}

int nemotoyz_attach_canvas(struct nemotoyz *toyz, int colortype, int alphatype, void *buffer, int width, int height)
{
	static SkColorType colortypes[] = {
		kN32_SkColorType
	};
	static SkAlphaType alphatypes[] = {
		kPremul_SkAlphaType,
		kUnpremul_SkAlphaType,
		kOpaque_SkAlphaType
	};

	if (toyz->bitmap != NULL)
		delete toyz->bitmap;
	if (toyz->device != NULL)
		delete toyz->device;
	if (toyz->canvas != NULL)
		delete toyz->canvas;

	toyz->bitmap = new SkBitmap;
	toyz->bitmap->setInfo(
			SkImageInfo::Make(
				width,
				height,
				colortypes[colortype],
				alphatypes[alphatype]));
	toyz->bitmap->setPixels(buffer);

	toyz->device = new SkBitmapDevice(*toyz->bitmap);
	toyz->canvas = new SkCanvas(toyz->device);

	return 0;
}

void nemotoyz_detach_canvas(struct nemotoyz *toyz)
{
	if (toyz->bitmap != NULL) {
		delete toyz->bitmap;

		toyz->bitmap = NULL;
	}

	if (toyz->device != NULL) {
		delete toyz->device;

		toyz->device = NULL;
	}

	if (toyz->canvas != NULL) {
		delete toyz->canvas;

		toyz->canvas = NULL;
	}
}

void nemotoyz_save(struct nemotoyz *toyz)
{
	toyz->canvas->save();
}

void nemotoyz_save_alpha(struct nemotoyz *toyz, float a)
{
	toyz->canvas->saveLayerAlpha(NULL, 0xff * a);
}

void nemotoyz_restore(struct nemotoyz *toyz)
{
	toyz->canvas->restore();
}

void nemotoyz_clear(struct nemotoyz *toyz)
{
	toyz->canvas->clear(SK_ColorTRANSPARENT);
}

void nemotoyz_clear_color(struct nemotoyz *toyz, float r, float g, float b, float a)
{
	toyz->canvas->clear(SkColorSetARGB(a, r, g, b));
}

void nemotoyz_identity(struct nemotoyz *toyz)
{
	toyz->canvas->resetMatrix();
}

void nemotoyz_translate(struct nemotoyz *toyz, float tx, float ty)
{
	toyz->canvas->translate(tx, ty);
}

void nemotoyz_scale(struct nemotoyz *toyz, float sx, float sy)
{
	toyz->canvas->scale(sx, sy);
}

void nemotoyz_rotate(struct nemotoyz *toyz, float rz)
{
	toyz->canvas->rotate(rz);
}

void nemotoyz_concat(struct nemotoyz *toyz, struct toyzmatrix *matrix)
{
	toyz->canvas->concat(*matrix->matrix);
}

void nemotoyz_matrix(struct nemotoyz *toyz, struct toyzmatrix *matrix)
{
	toyz->canvas->setMatrix(*matrix->matrix);
}

void nemotoyz_clip_rectangle(struct nemotoyz *toyz, float x, float y, float w, float h)
{
	toyz->canvas->clipRect(SkRect::MakeXYWH(x, y, w, h));
}

void nemotoyz_clip_region(struct nemotoyz *toyz, struct toyzregion *region)
{
	toyz->canvas->clipRegion(*region->region);
}

void nemotoyz_clip_path(struct nemotoyz *toyz, struct toyzpath *path)
{
	toyz->canvas->clipPath(*path->path);
}
