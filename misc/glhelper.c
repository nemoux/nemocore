#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <glhelper.h>

GLuint glshader_compile(GLenum type, int count, const char **sources)
{
	GLint status;
	GLuint s;

	s = glCreateShader(type);
	glShaderSource(s, count, sources, NULL);
	glCompileShader(s);

	glGetShaderiv(s, GL_COMPILE_STATUS, &status);
	if (!status) {
		GLsizei len;
		char log[1000];

		glGetShaderInfoLog(s, 1000, &len, log);
		fprintf(stderr, "Error: compiling:\n%*s\n", len, log);

		return GL_NONE;
	}

	return s;
}

int glshader_prepare(struct glshader *shader, const char *vertex_source, const char *fragment_source, int debug)
{
	const char *sources[3];
	GLint status;
	int count;

	shader->vertex_shader = glshader_compile(GL_VERTEX_SHADER, 1, &vertex_source);

	if (debug != 0) {
		sources[0] = fragment_source;
		sources[1] = GLHELPER_FRAGMENT_DEBUG;
		sources[2] = GLHELPER_FRAGMENT_BRACE;
		count = 3;
	} else {
		sources[0] = fragment_source;
		sources[1] = GLHELPER_FRAGMENT_BRACE;
		count = 2;
	}

	shader->fragment_shader = glshader_compile(GL_FRAGMENT_SHADER, count, sources);

	shader->program = glCreateProgram();
	glAttachShader(shader->program, shader->vertex_shader);
	glAttachShader(shader->program, shader->fragment_shader);
	glBindAttribLocation(shader->program, 0, "position");
	glBindAttribLocation(shader->program, 1, "texcoord");

	glLinkProgram(shader->program);
	glGetProgramiv(shader->program, GL_LINK_STATUS, &status);
	if (!status) {
		GLsizei len;
		char log[1000];

		glGetProgramInfoLog(shader->program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%*s\n", len, log);

		return -1;
	}

	shader->uprojection = glGetUniformLocation(shader->program, "proj");
	shader->utextures[0] = glGetUniformLocation(shader->program, "tex");
	shader->utextures[1] = glGetUniformLocation(shader->program, "tex1");
	shader->utextures[2] = glGetUniformLocation(shader->program, "tex2");
	shader->ualpha = glGetUniformLocation(shader->program, "alpha");
	shader->ucolor = glGetUniformLocation(shader->program, "color");

	return 0;
}

void glshader_finish(struct glshader *shader)
{
	glDeleteShader(shader->vertex_shader);
	glDeleteShader(shader->fragment_shader);
	glDeleteProgram(shader->program);

	shader->vertex_shader = 0;
	shader->fragment_shader = 0;
	shader->program = 0;
}

GLuint glshader_create_program(const char *fshader, const char *vshader)
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
		GLsizei len;
		char log[1000];

		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%*s\n", len, log);

		return GL_NONE;
	}

	return program;
}
