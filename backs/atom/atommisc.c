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
