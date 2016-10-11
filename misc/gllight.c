#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <gllight.h>
#include <glhelper.h>
#include <fbohelper.h>
#include <oshelper.h>
#include <nemomisc.h>

static const char GLLIGHT_GEOMETRY_VERTEX_SHADER[] =
"attribute vec3 position;\n"
"attribute vec3 normal;\n"
"attribute vec2 texcoord;\n"
"varying vec4 vnormal;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 1.0);\n"
"  vnormal = vec4(normal, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLLIGHT_GEOMETRY_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec4 vnormal;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"void main()\n"
"{\n"
"  gl_FragData[0] = vec4(texture2D(tex, vtexcoord).rgb, 0.0);\n"
"  gl_FragData[1] = vec4(vnormal.xyz, 0.0);\n"
"}\n";

static const char GLLIGHT_LIGHT_VERTEX_SHADER[] =
"attribute vec3 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLLIGHT_LIGHT_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tdiffuse;\n"
"uniform sampler2D tnormal;\n"
"uniform sampler2D tdepth;\n"
"uniform vec3 lposition;\n"
"uniform vec3 lcolor;\n"
"uniform float lsize;\n"
"void main()\n"
"{\n"
"  vec4 diffuse = texture2D(tdiffuse, vtexcoord);\n"
"  vec4 normal = texture2D(tnormal, vtexcoord);\n"
"  float depth = texture2D(tdepth, vtexcoord).r;\n"
"  vec4 position = vec4(vtexcoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);\n"
"  vec3 ldir = lposition - position.xyz;\n"
"  float attenuation = 1.0 - pow(clamp(length(ldir) / lsize, 0.0, 1.0), 2.0);\n"
"  ldir = normalize(ldir);\n"
"  float dot = clamp(dot(ldir, normal.xyz), 0.0, 1.0);\n"
"  vec3 color = attenuation * lcolor * 1.0;\n"
"  gl_FragColor = vec4(color * diffuse.rgb, 1.0);\n"
"}\n";

static GLuint gllight_create_program(const char *vshader, const char *fshader)
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

struct gllight *gllight_create(int32_t width, int32_t height)
{
	struct gllight *light;

	light = (struct gllight *)malloc(sizeof(struct gllight));
	if (light == NULL)
		return NULL;
	memset(light, 0, sizeof(struct gllight));

	glGenTextures(1, &light->texture);
	glBindTexture(GL_TEXTURE_2D, light->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &light->gbuffer0);
	glBindTexture(GL_TEXTURE_2D, light->gbuffer0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &light->gbuffer1);
	glBindTexture(GL_TEXTURE_2D, light->gbuffer1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &light->dbuffer);
	glBindTexture(GL_TEXTURE_2D, light->dbuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	light->geometry.program = gllight_create_program(GLLIGHT_GEOMETRY_VERTEX_SHADER, GLLIGHT_GEOMETRY_FRAGMENT_SHADER);
	if (light->geometry.program == 0)
		goto err1;
	light->light.program = gllight_create_program(GLLIGHT_LIGHT_VERTEX_SHADER, GLLIGHT_LIGHT_FRAGMENT_SHADER);
	if (light->light.program == 0)
		goto err2;

	light->geometry.utexture = glGetUniformLocation(light->geometry.program, "tex");

	light->light.udiffuse = glGetUniformLocation(light->light.program, "tdiffuse");
	light->light.unormal = glGetUniformLocation(light->light.program, "tnormal");
	light->light.udepth = glGetUniformLocation(light->light.program, "tdepth");
	light->light.uposition = glGetUniformLocation(light->light.program, "lposition");
	light->light.ucolor = glGetUniformLocation(light->light.program, "lcolor");
	light->light.usize = glGetUniformLocation(light->light.program, "lsize");

	fbo_prepare_context(light->texture, width, height, &light->fbo, &light->dbo);

	glGenFramebuffers(1, &light->gfbo);
	glBindFramebuffer(GL_FRAMEBUFFER, light->gfbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, light->gbuffer0, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, light->gbuffer1, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, light->dbuffer, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	light->width = width;
	light->height = height;

	return light;

err3:
	glDeleteProgram(light->geometry.program);
	glDeleteProgram(light->light.program);

err2:
	glDeleteProgram(light->geometry.program);

err1:
	glDeleteTextures(1, &light->gbuffer0);
	glDeleteTextures(1, &light->gbuffer1);
	glDeleteTextures(1, &light->dbuffer);

	glDeleteTextures(1, &light->texture);

	free(light);

	return NULL;
}

void gllight_destroy(struct gllight *light)
{
	glDeleteTextures(1, &light->texture);

	glDeleteFramebuffers(1, &light->fbo);
	glDeleteRenderbuffers(1, &light->dbo);

	glDeleteTextures(1, &light->gbuffer0);
	glDeleteTextures(1, &light->gbuffer1);
	glDeleteTextures(1, &light->dbuffer);

	glDeleteFramebuffers(1, &light->gfbo);

	glDeleteProgram(light->geometry.program);
	glDeleteProgram(light->light.program);

	free(light);
}

void gllight_set_pointlight_position(struct gllight *light, int index, float x, float y, float z)
{
	light->pointlights[index].position[0] = x;
	light->pointlights[index].position[1] = y;
	light->pointlights[index].position[2] = z;
}

void gllight_set_pointlight_color(struct gllight *light, int index, float r, float g, float b)
{
	light->pointlights[index].color[0] = r;
	light->pointlights[index].color[1] = g;
	light->pointlights[index].color[2] = b;
}

void gllight_set_pointlight_size(struct gllight *light, int index, float size)
{
	light->pointlights[index].size = size;
}

void gllight_resize(struct gllight *light, int32_t width, int32_t height)
{
	if (light->width != width || light->height != height) {
		glBindTexture(GL_TEXTURE_2D, light->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &light->fbo);
		glDeleteRenderbuffers(1, &light->dbo);

		fbo_prepare_context(light->texture, width, height, &light->fbo, &light->dbo);

		glBindTexture(GL_TEXTURE_2D, light->gbuffer0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_2D, light->gbuffer1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_2D, light->dbuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &light->gfbo);

		glGenFramebuffers(1, &light->gfbo);
		glBindFramebuffer(GL_FRAMEBUFFER, light->gfbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, light->gbuffer0, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, light->gbuffer1, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, light->dbuffer, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		light->width = width;
		light->height = height;
	}
}

static void gllight_render_geometry(struct gllight *light, GLuint texture)
{
	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f
	};
	static GLenum buffers[] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2
	};

	glBindFramebuffer(GL_FRAMEBUFFER, light->gfbo);

	glEnable(GL_DEPTH);
	glDepthMask(GL_TRUE);

	glViewport(0, 0, light->width, light->height);

	glDrawBuffers(2, buffers);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(light->geometry.program);
	glUniform1i(light->geometry.utexture, 0);

	glBindTexture(GL_TEXTURE_2D, texture);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), &vertices[3]);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), &vertices[6]);
	glEnableVertexAttribArray(2);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void gllight_render_light(struct gllight *light)
{
	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 0.0f, 1.0f
	};
	static GLenum buffers[] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2
	};
	int i;

	glBindFramebuffer(GL_FRAMEBUFFER, light->fbo);

	glViewport(0, 0, light->width, light->height);

	glDrawBuffers(1, buffers);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDepthMask(GL_FALSE);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(light->light.program);
	glUniform1i(light->light.udiffuse, 0);
	glUniform1i(light->light.unormal, 1);
	glUniform1i(light->light.udepth, 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, light->gbuffer0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, light->gbuffer1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, light->dbuffer);

	for (i = 0; i < GLLIGHT_POINTLIGHTS_MAX; i++) {
		if (light->pointlights[i].size > 0.0f) {
			glUniform3fv(light->light.uposition, 1, light->pointlights[i].position);
			glUniform3fv(light->light.ucolor, 1, light->pointlights[i].color);
			glUniform1f(light->light.usize, light->pointlights[i].size);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &vertices[0]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &vertices[3]);
			glEnableVertexAttribArray(1);

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void gllight_dispatch(struct gllight *light, GLuint texture)
{
	gllight_render_geometry(light, texture);
	gllight_render_light(light);
}
