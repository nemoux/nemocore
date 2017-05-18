#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <glhelper.h>
#include <pixmanhelper.h>

GLuint gl_compile_shader(GLenum type, int count, const char **sources)
{
	GLuint shader;
	GLint status;

	shader = glCreateShader(type);
	glShaderSource(shader, count, sources, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		GLsizei len;
		char log[1000];

		glGetShaderInfoLog(shader, 1000, &len, log);
		fprintf(stderr, "Error: compiling:\n%*s\n", len, log);

		return GL_NONE;
	}

	return shader;
}

GLuint gl_compile_program(const char *vertex_source, const char *fragment_source, GLuint *vertex_shader, GLuint *fragment_shader)
{
	GLuint program;
	GLuint vshader;
	GLuint fshader;
	GLint status;

	vshader = gl_compile_shader(GL_FRAGMENT_SHADER, 1, &fragment_source);
	fshader = gl_compile_shader(GL_VERTEX_SHADER, 1, &vertex_source);

	program = glCreateProgram();
	glAttachShader(program, vshader);
	glAttachShader(program, fshader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		GLsizei len;
		char log[1000];

		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%*s\n", len, log);

		return GL_NONE;
	}

	if (vertex_shader != NULL)
		*vertex_shader = vshader;
	if (fragment_shader != NULL)
		*fragment_shader = fshader;

	return program;
}

GLuint gl_create_texture(GLint filter, GLint wrap, GLuint width, GLuint height)
{
	GLuint texture;

	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

int gl_load_texture(GLuint texture, GLuint width, GLuint height, const char *filepath)
{
	pixman_image_t *image;

	image = pixman_load_image(filepath, width, height);
	if (image == NULL)
		return -1;

	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_BGRA,
			pixman_image_get_stride(image),
			pixman_image_get_height(image),
			0,
			GL_BGRA,
			GL_UNSIGNED_BYTE,
			(void *)pixman_image_get_data(image));
	glBindTexture(GL_TEXTURE_2D, 0);

	pixman_image_unref(image);

	return 0;
}

int gl_create_fbo(GLuint tex, GLuint width, GLuint height, GLuint *fbo, GLuint *dbo)
{
	GLenum status;

	glGenFramebuffers(1, fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

	glGenRenderbuffers(1, dbo);
	glBindRenderbuffer(GL_RENDERBUFFER, *dbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, *dbo);

	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		return -1;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return 0;
}
