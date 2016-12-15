#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cookshader.h>
#include <nemomisc.h>
#include <glhelper.h>

struct cookshader *nemocook_shader_create(void)
{
	struct cookshader *shader;

	shader = (struct cookshader *)malloc(sizeof(struct cookshader));
	if (shader == NULL)
		return NULL;
	memset(shader, 0, sizeof(struct cookshader));

	return shader;
}

void nemocook_shader_destroy(struct cookshader *shader)
{
	if (shader->program > 0) {
		glDeleteShader(shader->vshader);
		glDeleteShader(shader->fshader);
		glDeleteProgram(shader->program);
	}

	free(shader);
}

int nemocook_shader_set_program(struct cookshader *shader, const char *vertex_source, const char *fragment_source)
{
	if (shader->program > 0) {
		glDeleteShader(shader->vshader);
		glDeleteShader(shader->fshader);
		glDeleteProgram(shader->program);
	}

	shader->program = gl_compile_program(vertex_source, fragment_source, &shader->vshader, &shader->fshader);
	if (shader->program == 0)
		return -1;

	return 0;
}

void nemocook_shader_set_attrib(struct cookshader *shader, int index, const char *name)
{
	glBindAttribLocation(shader->program, index, name);
}

void nemocook_shader_set_uniform(struct cookshader *shader, int index, const char *name)
{
	shader->uniforms[index] = glGetUniformLocation(shader->program, name);
}

void nemocook_shader_set_uniform_1i(struct cookshader *shader, int index, int value)
{
	glUniform1i(shader->uniforms[index], value);
}

void nemocook_shader_set_uniform_1f(struct cookshader *shader, int index, float value)
{
	glUniform1f(shader->uniforms[index], value);
}

void nemocook_shader_set_uniform_2fv(struct cookshader *shader, int index, float *value)
{
	glUniform2fv(shader->uniforms[index], 1, value);
}

void nemocook_shader_set_uniform_matrix4fv(struct cookshader *shader, int index, float *value)
{
	glUniformMatrix4fv(shader->uniforms[index], 1, GL_FALSE, value);
}

void nemocook_shader_set_uniform_4fv(struct cookshader *shader, int index, float *value)
{
	glUniform4fv(shader->uniforms[index], 1, value);
}
