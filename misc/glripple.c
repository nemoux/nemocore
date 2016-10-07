#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <glripple.h>
#include <glhelper.h>
#include <fbohelper.h>
#include <nemomisc.h>

static GLuint glripple_create_program(void)
{
	static const char GLRIPPLE_SIMPLE_VERTEX_SHADER[] =
		"attribute vec3 position;\n"
		"attribute vec2 texcoord;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vec4(position, 1.0);\n"
		"  vtexcoord = texcoord;\n"
		"}\n";
	static const char GLRIPPLE_SIMPLE_FRAGMENT_SHADER[] =
		"precision mediump float;\n"
		"varying vec2 vtexcoord;\n"
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(tex, vtexcoord);\n"
		"}\n";

	const char *vertexshader = GLRIPPLE_SIMPLE_VERTEX_SHADER;
	const char *fragmentshader = GLRIPPLE_SIMPLE_FRAGMENT_SHADER;
	GLuint frag, vert;
	GLuint program;
	GLint status;

	frag = glshader_compile(GL_FRAGMENT_SHADER, 1, &fragmentshader);
	vert = glshader_compile(GL_VERTEX_SHADER, 1, &vertexshader);

	program = glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		GLsizei len;
		char log[1000];

		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%*s\n", len, log);

		return 0;
	}

	glUseProgram(program);
	glBindAttribLocation(program, 0, "position");
	glBindAttribLocation(program, 1, "texcoord");

	return program;
}

struct glripple *glripple_create(int32_t width, int32_t height)
{
	struct glripple *ripple;

	ripple = (struct glripple *)malloc(sizeof(struct glripple));
	if (ripple == NULL)
		return NULL;
	memset(ripple, 0, sizeof(struct glripple));

	ripple->program = glripple_create_program();
	if (ripple->program == 0)
		goto err1;

	ripple->utexture = glGetUniformLocation(ripple->program, "tex");

	glGenTextures(1, &ripple->texture);
	glBindTexture(GL_TEXTURE_2D, ripple->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenVertexArrays(1, &ripple->varray);
	glGenBuffers(1, &ripple->vvertex);
	glGenBuffers(1, &ripple->vtexcoord);
	glGenBuffers(1, &ripple->vindex);

	fbo_prepare_context(ripple->texture, width, height, &ripple->fbo, &ripple->dbo);

	ripple->width = width;
	ripple->height = height;

	return ripple;

err1:
	free(ripple);

	return NULL;
}

void glripple_destroy(struct glripple *ripple)
{
	glDeleteTextures(1, &ripple->texture);

	glDeleteBuffers(1, &ripple->vvertex);
	glDeleteBuffers(1, &ripple->vtexcoord);
	glDeleteBuffers(1, &ripple->vindex);
	glDeleteVertexArrays(1, &ripple->varray);

	glDeleteFramebuffers(1, &ripple->fbo);
	glDeleteRenderbuffers(1, &ripple->dbo);

	glDeleteProgram(ripple->program);

	free(ripple);
}

void glripple_layout(struct glripple *ripple, int32_t rows, int32_t columns)
{
	GLfloat *vertices;
	GLfloat *texcoords;
	GLuint *indices;
	GLfloat ox = -1.0f;
	GLfloat oy = -1.0f;
	GLfloat px = 2.0f / columns;
	GLfloat py = 2.0f / rows;
	int idx;
	int x, y;

	vertices = (GLfloat *)malloc(sizeof(GLfloat[3]) * (rows + 1) * (columns + 1));
	texcoords = (GLfloat *)malloc(sizeof(GLfloat[2]) * (rows + 1) * (columns + 1));
	indices = (GLuint *)malloc(sizeof(GLuint[2]) * rows * (columns + 1));

	for (y = 0; y <= rows; y++) {
		for (x = 0; x <= columns; x++) {
			idx = y * (columns + 1) + x;

			vertices[idx * 3 + 0] = ox + (float)x * px;
			vertices[idx * 3 + 1] = oy + (float)y * py;
			vertices[idx * 3 + 2] = 0.0f;

			texcoords[idx * 2 + 0] = (float)x / (float)columns;
			texcoords[idx * 2 + 1] = (float)y / (float)rows;
		}
	}

	for (y = 0; y < rows; y++) {
		for (x = 0; x <= columns; x++) {
			idx = y * (columns + 1) + x;

			indices[idx * 2 + 0] = y * (columns + 1) + x;
			indices[idx * 2 + 1] = (y + 1) * (columns + 1) + x;
		}
	}

	glBindVertexArray(ripple->varray);

	glBindBuffer(GL_ARRAY_BUFFER, ripple->vvertex);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat[3]) * (rows + 1) * (columns + 1), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, ripple->vtexcoord);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat[2]) * (rows + 1) * (columns + 1), texcoords, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ripple->vindex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint[2]) * rows * (columns + 1), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	free(vertices);
	free(texcoords);
	free(indices);

	ripple->rows = rows;
	ripple->columns = columns;
	ripple->elements = rows * (columns + 1) * 2;
}

void glripple_resize(struct glripple *ripple, int32_t width, int32_t height)
{
	if (ripple->width != width || ripple->height != height) {
		glBindTexture(GL_TEXTURE_2D, ripple->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &ripple->fbo);
		glDeleteRenderbuffers(1, &ripple->dbo);

		fbo_prepare_context(ripple->texture, width, height, &ripple->fbo, &ripple->dbo);

		ripple->width = width;
		ripple->height = height;
	}
}

void glripple_dispatch(struct glripple *ripple, GLuint texture)
{
	glBindFramebuffer(GL_FRAMEBUFFER, ripple->fbo);

	glViewport(0, 0, ripple->width, ripple->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(ripple->program);
	glUniform1i(ripple->program, 0);

	glBindTexture(GL_TEXTURE_2D, texture);

	glBindVertexArray(ripple->varray);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ripple->vindex);

	glDrawElements(GL_TRIANGLE_STRIP, ripple->elements, GL_UNSIGNED_INT, (void *)0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
