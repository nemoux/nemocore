#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <glshader.h>

GLuint glshader_compile(GLenum type, int count, const char **sources)
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

GLuint glshader_compile_program(const char *vertex_source, const char *fragment_source, GLuint *vertex_shader, GLuint *fragment_shader)
{
	GLuint program;
	GLuint vshader;
	GLuint fshader;
	GLint status;

	vshader = glshader_compile(GL_FRAGMENT_SHADER, 1, &fragment_source);
	fshader = glshader_compile(GL_VERTEX_SHADER, 1, &vertex_source);

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
