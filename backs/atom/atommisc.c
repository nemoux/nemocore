#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <atomback.h>
#include <atommisc.h>
#include <pixmanhelper.h>
#include <nemomisc.h>

const char *simple_vertex_shader =
"uniform mat4 matrix;\n"
"attribute vec3 vertex;\n"
"attribute vec2 texcoord;\n"
"varying vec2 v_texcoord;\n"
"void main() {\n"
"  gl_Position = matrix * vec4(vertex, 1.0);\n"
"  v_texcoord = texcoord;\n"
"}\n";

const char *simple_fragment_shader =
"precision mediump float;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  gl_FragColor = color;\n"
"}\n";

const char *texture_fragment_shader =
"precision mediump float;\n"
"varying vec2 v_texcoord;\n"
"uniform sampler2D tex0;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  gl_FragColor = texture2D(tex0, v_texcoord) * color;\n"
"}\n";

GLuint nemoback_atom_create_shader(const char *fshader, const char *vshader)
{
	GLuint frag, vert;
	GLuint program;
	GLint status;

	frag = glshader_compile(GL_FRAGMENT_SHADER, 1, &fshader);
	vert = glshader_compile(GL_VERTEX_SHADER, 1, &vshader);

	program = glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%*s\n", len, log);
		exit(1);
	}

	glUseProgram(program);

	glBindAttribLocation(program, 0, "vertex");
	glBindAttribLocation(program, 1, "texcoord");
	glLinkProgram(program);

	return program;
}

void nemoback_atom_prepare_shader(struct atomback *atom, GLuint program)
{
	if (atom->program == program)
		return;

	atom->program = program;

	atom->umatrix = glGetUniformLocation(atom->program, "matrix");
	atom->ucolor = glGetUniformLocation(atom->program, "color");

	atom->utex0 = glGetUniformLocation(atom->program, "tex0");
}

void nemoback_atom_create_buffer(struct atomback *atom)
{
	glGenVertexArrays(1, &atom->varray);
	glGenBuffers(1, &atom->vbuffer);
	glGenBuffers(1, &atom->vindex);
}

void nemoback_atom_prepare_buffer(struct atomback *atom, GLenum mode, float *buffers, int elements)
{
	glBindVertexArray(atom->varray);

	glBindBuffer(GL_ARRAY_BUFFER, atom->vbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)sizeof(GLfloat[3]));
	glEnableVertexAttribArray(1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * elements, buffers, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	atom->mode = mode;
	atom->elements = elements / 5;
}

void nemoback_atom_prepare_index(struct atomback *atom, GLenum mode, uint32_t *buffers, int elements)
{
	glBindVertexArray(atom->varray);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, atom->vindex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * elements, buffers, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	atom->mode = mode;
	atom->elements = elements / 5;
}

GLuint nemoback_atom_create_texture_from_image(const char *filepath)
{
	pixman_image_t *image;
	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	image = pixman_load_png_file(filepath);
	if (image == NULL)
		image = pixman_load_jpeg_file(filepath);
	if (image == NULL)
		exit(1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
			pixman_image_get_width(image),
			pixman_image_get_height(image),
			0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
			pixman_image_get_data(image));
	pixman_image_unref(image);

	return texture;
}
