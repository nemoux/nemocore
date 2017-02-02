#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotoyz.h>
#include <toyzpicture.hpp>
#include <nemomisc.h>

struct toyzpicture *nemotoyz_picture_create(void)
{
	struct toyzpicture *picture;

	picture = new toyzpicture;
	picture->picture = NULL;

	return picture;
}

void nemotoyz_picture_destroy(struct toyzpicture *picture)
{
	delete picture;
}

void nemotoyz_picture_save(struct toyzpicture *picture, const char *url)
{
	SkFILEWStream stream(url);

	picture->picture->serialize(&stream);
}

void nemotoyz_picture_load(struct toyzpicture *picture, const char *url)
{
	SkFILEStream stream(url);

	if (stream.isValid())
		picture->picture = SkPicture::MakeFromStream(&stream);
}
