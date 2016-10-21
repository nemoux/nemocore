#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <gllight.h>
#include <glshader.h>
#include <fbohelper.h>
#include <oshelper.h>
#include <nemomisc.h>

static const char GLLIGHT_LIGHT_VERTEX_SHADER[] =
"attribute vec3 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLLIGHT_AMBIENT_LIGHT_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tdiffuse;\n"
"uniform vec3 lambient;\n"
"void main()\n"
"{\n"
"  vec4 diffuse = texture2D(tdiffuse, vtexcoord);\n"
"  gl_FragColor = vec4(lambient * diffuse.rgb, 1.0);\n"
"}\n";

static const char GLLIGHT_POINT_LIGHT_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tdiffuse;\n"
"uniform vec3 lposition;\n"
"uniform vec3 lcolor;\n"
"uniform float lsize;\n"
"uniform float lscope;\n"
"uniform float time;\n"
"vec3 ball(vec2 p, vec2 o, vec3 c, float s)\n"
"{\n"
"  vec2 r = vec2(o.x - p.x, o.y - p.y);\n"
"  return pow(s / length(r), 1.0) * c;\n"
"}\n"
"void main()\n"
"{\n"
"  vec4 diffuse = texture2D(tdiffuse, vtexcoord);\n"
"  vec4 position = vec4(vtexcoord * 2.0 - 1.0, 0.0, 1.0);\n"
"  vec3 ldir = lposition - position.xyz;\n"
"  float attenuation = 1.0 - pow(clamp(length(ldir) / lscope, 0.0, 1.0), 2.0);\n"
"  vec3 color = attenuation * lcolor;\n"
"  gl_FragColor = vec4(color * diffuse.rgb + ball(position.xy, lposition.xy, lcolor, lsize), 1.0);\n"
"}\n";

static GLuint nemofx_gllight_create_program(const char *vshader, const char *fshader)
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

		return 0;
	}

	glUseProgram(program);
	glBindAttribLocation(program, 0, "position");
	glBindAttribLocation(program, 1, "texcoord");

	return program;
}

struct gllight *nemofx_gllight_create(int32_t width, int32_t height)
{
	struct gllight *light;

	light = (struct gllight *)malloc(sizeof(struct gllight));
	if (light == NULL)
		return NULL;
	memset(light, 0, sizeof(struct gllight));

	glGenTextures(1, &light->texture);
	glBindTexture(GL_TEXTURE_2D, light->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	light->program0 = nemofx_gllight_create_program(GLLIGHT_LIGHT_VERTEX_SHADER, GLLIGHT_AMBIENT_LIGHT_FRAGMENT_SHADER);
	if (light->program0 == 0)
		goto err1;
	light->program1 = nemofx_gllight_create_program(GLLIGHT_LIGHT_VERTEX_SHADER, GLLIGHT_POINT_LIGHT_FRAGMENT_SHADER);
	if (light->program1 == 0)
		goto err2;

	light->udiffuse0 = glGetUniformLocation(light->program0, "tdiffuse");
	light->uambient0 = glGetUniformLocation(light->program0, "lambient");

	light->udiffuse1 = glGetUniformLocation(light->program1, "tdiffuse");
	light->uposition1 = glGetUniformLocation(light->program1, "lposition");
	light->ucolor1 = glGetUniformLocation(light->program1, "lcolor");
	light->usize1 = glGetUniformLocation(light->program1, "lsize");
	light->uscope1 = glGetUniformLocation(light->program1, "lscope");
	light->utime1 = glGetUniformLocation(light->program1, "time");

	fbo_prepare_context(light->texture, width, height, &light->fbo, &light->dbo);

	light->width = width;
	light->height = height;

	return light;

err2:
	glDeleteProgram(light->program0);

err1:
	glDeleteTextures(1, &light->texture);

	free(light);

	return NULL;
}

void nemofx_gllight_destroy(struct gllight *light)
{
	glDeleteTextures(1, &light->texture);

	glDeleteFramebuffers(1, &light->fbo);
	glDeleteRenderbuffers(1, &light->dbo);

	glDeleteProgram(light->program0);
	glDeleteProgram(light->program1);

	free(light);
}

void nemofx_gllight_set_ambientlight_color(struct gllight *light, float r, float g, float b)
{
	light->ambientlight.color[0] = r;
	light->ambientlight.color[1] = g;
	light->ambientlight.color[2] = b;
}

void nemofx_gllight_set_pointlight_position(struct gllight *light, int index, float x, float y)
{
	light->pointlights[index].position[0] = x * 2.0f - 1.0f;
	light->pointlights[index].position[1] = y * 2.0f - 1.0f;
	light->pointlights[index].position[2] = 0.0f;
}

void nemofx_gllight_set_pointlight_color(struct gllight *light, int index, float r, float g, float b)
{
	light->pointlights[index].color[0] = r;
	light->pointlights[index].color[1] = g;
	light->pointlights[index].color[2] = b;
}

void nemofx_gllight_set_pointlight_size(struct gllight *light, int index, float size)
{
	light->pointlights[index].size = size;
}

void nemofx_gllight_set_pointlight_scope(struct gllight *light, int index, float scope)
{
	light->pointlights[index].scope = scope;
}

void nemofx_gllight_clear_pointlights(struct gllight *light)
{
	int i;

	for (i = 0; i < GLLIGHT_POINTLIGHTS_MAX; i++) {
		light->pointlights[i].scope = 0.0f;
	}
}

void nemofx_gllight_resize(struct gllight *light, int32_t width, int32_t height)
{
	if (light->width != width || light->height != height) {
		glBindTexture(GL_TEXTURE_2D, light->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &light->fbo);
		glDeleteRenderbuffers(1, &light->dbo);

		fbo_prepare_context(light->texture, width, height, &light->fbo, &light->dbo);

		light->width = width;
		light->height = height;
	}
}

void nemofx_gllight_dispatch(struct gllight *light, GLuint texture)
{
	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 0.0f, 1.0f
	};
	float secs = (float)time_current_nsecs() / 1000000000.0f;
	int i;

	glBindFramebuffer(GL_FRAMEBUFFER, light->fbo);

	glViewport(0, 0, light->width, light->height);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDepthMask(GL_FALSE);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, texture);

	glUseProgram(light->program0);
	glUniform1i(light->udiffuse0, 0);
	glUniform3fv(light->uambient0, 1, light->ambientlight.color);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &vertices[3]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glUseProgram(light->program1);
	glUniform1i(light->udiffuse1, 0);
	glUniform1f(light->utime1, secs);

	for (i = 0; i < GLLIGHT_POINTLIGHTS_MAX; i++) {
		if (light->pointlights[i].scope > 0.0f) {
			glUniform3fv(light->uposition1, 1, light->pointlights[i].position);
			glUniform3fv(light->ucolor1, 1, light->pointlights[i].color);
			glUniform1f(light->usize1, light->pointlights[i].size);
			glUniform1f(light->uscope1, light->pointlights[i].scope);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &vertices[0]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &vertices[3]);
			glEnableVertexAttribArray(1);

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
