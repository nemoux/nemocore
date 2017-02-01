#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotoyz.hpp>
#include <nemomisc.h>

struct nemotoyz *nemotoyz_create(void)
{
	struct nemotoyz *toyz;

	toyz = new nemotoyz;
	toyz->bitmap = NULL;

	return toyz;
}

void nemotoyz_destroy(struct nemotoyz *toyz)
{
	if (toyz->bitmap != NULL)
		delete toyz->bitmap;

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

	toyz->bitmap = new SkBitmap;
	toyz->bitmap->setInfo(
			SkImageInfo::Make(
				width,
				height,
				colortypes[colortype],
				alphatypes[alphatype]));
	toyz->bitmap->setPixels(buffer);

	return 0;
}

void nemotoyz_detach_canvas(struct nemotoyz *toyz)
{
	if (toyz->bitmap != NULL) {
		delete toyz->bitmap;

		toyz->bitmap = NULL;
	}
}
