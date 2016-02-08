#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <atomback.h>
#include <atommisc.h>
#include <nemomisc.h>

const char *simple_vertex_shader =
"uniform mat4 matrix;\n"
"attribute vec3 vertex;\n"
"void main() {\n"
"  gl_Position = matrix * vec4(vertex, 1.0);\n"
"}\n";

const char *simple_fragment_shader =
"precision mediump float;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  gl_FragColor = color;\n"
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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * elements, buffers, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	atom->mode = mode;
	atom->elements = elements / 3;
}

void nemoback_atom_prepare_index(struct atomback *atom, GLenum mode, uint32_t *buffers, int elements)
{
	glBindVertexArray(atom->varray);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, atom->vindex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * elements, buffers, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	atom->mode = mode;
	atom->elements = elements / 3;
}
