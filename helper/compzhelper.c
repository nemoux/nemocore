#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <compzhelper.h>
#include <glhelper.h>

int glcompz_prepare(struct glcompz *shader, const char *vertex_source, const char *fragment_source)
{
	GLint status;

	shader->vertex_shader = gl_compile_shader(GL_VERTEX_SHADER, 1, &vertex_source);
	shader->fragment_shader = gl_compile_shader(GL_FRAGMENT_SHADER, 1, &fragment_source);

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

	shader->uprojection = glGetUniformLocation(shader->program, "projection");
	shader->utextures[0] = glGetUniformLocation(shader->program, "tex");
	shader->utextures[1] = glGetUniformLocation(shader->program, "tex1");
	shader->utextures[2] = glGetUniformLocation(shader->program, "tex2");
	shader->ualpha = glGetUniformLocation(shader->program, "alpha");
	shader->ucolor = glGetUniformLocation(shader->program, "color");

	return 0;
}

void glcompz_finish(struct glcompz *shader)
{
	glDeleteShader(shader->vertex_shader);
	glDeleteShader(shader->fragment_shader);
	glDeleteProgram(shader->program);

	shader->vertex_shader = 0;
	shader->fragment_shader = 0;
	shader->program = 0;
}
