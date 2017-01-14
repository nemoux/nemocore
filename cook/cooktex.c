#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cooktex.h>
#include <nemomisc.h>

struct cooktex *nemocook_texture_create(void)
{
	struct cooktex *tex;

	tex = (struct cooktex *)malloc(sizeof(struct cooktex));
	if (tex == NULL)
		return NULL;
	memset(tex, 0, sizeof(struct cooktex));

	nemocook_one_prepare(&tex->one);

	return tex;
}

void nemocook_texture_destroy(struct cooktex *tex)
{
	nemocook_one_finish(&tex->one);

	if (tex->is_mine != 0)
		glDeleteTextures(1, &tex->texture);

	if (tex->pbo > 0)
		glDeleteBuffers(1, &tex->pbo);

	free(tex);
}

void nemocook_texture_assign(struct cooktex *tex, int format, int width, int height)
{
	tex->width = width;
	tex->height = height;

	if (format == NEMOCOOK_TEXTURE_BGRA_FORMAT) {
		tex->format = GL_BGRA;
		tex->bpp = 4;
	} else if (format == NEMOCOOK_TEXTURE_LUMINANCE_FORMAT) {
		tex->format = GL_LUMINANCE;
		tex->bpp = 1;
	}

	glGenTextures(1, &tex->texture);
	glBindTexture(GL_TEXTURE_2D, tex->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, tex->format, width, height, 0, tex->format, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	tex->is_mine = 1;
}

void nemocook_texture_resize(struct cooktex *tex, int width, int height)
{
	if (tex->width != width || tex->height != height) {
		tex->width = width;
		tex->height = height;

		glBindTexture(GL_TEXTURE_2D, tex->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, tex->format, width, height, 0, tex->format, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		if (tex->pbo > 0) {
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, tex->pbo);
			glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * tex->bpp, NULL, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		}
	}
}

void nemocook_texture_set_filter(struct cooktex *tex, int filter)
{
	static GLuint glfilters[] = {
		GL_LINEAR,
		GL_NEAREST
	};

	glBindTexture(GL_TEXTURE_2D, tex->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glfilters[filter]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glfilters[filter]);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void nemocook_texture_set_wrap(struct cooktex *tex, int wrap)
{
	static GLuint glwraps[] = {
		GL_CLAMP_TO_EDGE,
		GL_CLAMP_TO_BORDER,
		GL_REPEAT,
		GL_MIRRORED_REPEAT
	};

	glBindTexture(GL_TEXTURE_2D, tex->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glwraps[wrap]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glwraps[wrap]);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void nemocook_texture_bind(struct cooktex *tex)
{
	glBindTexture(GL_TEXTURE_2D, tex->texture);
}

void nemocook_texture_unbind(struct cooktex *tex)
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void nemocook_texture_upload(struct cooktex *tex, void *buffer)
{
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, tex->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, tex->format, tex->width, tex->height, 0, tex->format, GL_UNSIGNED_BYTE, buffer);
}

void nemocook_texture_upload_slice(struct cooktex *tex, void *buffer, int x, int y, int w, int h)
{
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, tex->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, x);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, y);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, tex->format, GL_UNSIGNED_BYTE, buffer);
}

void *nemocook_texture_map(struct cooktex *tex)
{
	if (tex->pbo == 0) {
		glGenBuffers(1, &tex->pbo);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, tex->pbo);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, tex->width * tex->height * tex->bpp, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, tex->pbo);

	return glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, tex->width * tex->height * tex->bpp, GL_MAP_WRITE_BIT);
}

void nemocook_texture_unmap(struct cooktex *tex)
{
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	glBindTexture(GL_TEXTURE_2D, tex->texture);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, tex->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, tex->format, tex->width, tex->height, 0, tex->format, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}
