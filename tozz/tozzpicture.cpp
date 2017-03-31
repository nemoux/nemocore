#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotozz.h>
#include <tozzpicture.hpp>
#include <nemomisc.h>

struct tozzpicture *nemotozz_picture_create(void)
{
	struct tozzpicture *picture;

	picture = new tozzpicture;
	picture->picture = NULL;

	return picture;
}

void nemotozz_picture_destroy(struct tozzpicture *picture)
{
	delete picture;
}

void nemotozz_picture_save(struct tozzpicture *picture, const char *url)
{
	SkFILEWStream stream(url);

	picture->picture->serialize(&stream);
}

void nemotozz_picture_load(struct tozzpicture *picture, const char *url)
{
	SkFILEStream stream(url);

	if (stream.isValid())
		picture->picture = SkPicture::MakeFromStream(&stream);
}
