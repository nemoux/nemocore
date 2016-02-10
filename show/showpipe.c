#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showpipe.h>
#include <showpoly.h>
#include <showcanvas.h>
#include <glhelper.h>
#include <nemomisc.h>

static const char *simple_vertex_shader =
"uniform mat4 projection;\n"
"uniform mat4 modelview;\n"
"attribute vec3 vertex;\n"
"void main() {\n"
"  gl_Position = projection * modelview * vec4(vertex, 1.0);\n"
"}\n";

static const char *simple_fragment_shader =
"precision mediump float;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  gl_FragColor = color;\n"
"}\n";

static const char *texture_vertex_shader =
"uniform mat4 projection;\n"
"uniform mat4 modelview;\n"
"attribute vec3 vertex;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main() {\n"
"  gl_Position = projection * modelview * vec4(vertex, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char *texture_fragment_shader =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex0;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  gl_FragColor = texture2D(tex0, vtexcoord) * color;\n"
"}\n";

static const char *light_vertex_shader =
"uniform mat4 projection;\n"
"uniform mat4 modelview;\n"
"uniform vec4 light;\n"
"attribute vec3 vertex;\n"
"attribute vec2 texcoord;\n"
"attribute vec3 normal;\n"
"varying vec3 vlight;\n"
"varying vec2 vtexcoord;\n"
"varying vec3 vnormal;\n"
"void main() {\n"
"  gl_Position = projection * modelview * vec4(vertex, 1.0);\n"
"  vlight = normalize(light.xyz - mat3(modelview) * vertex);\n"
"  vtexcoord = texcoord;\n"
"  vnormal = normalize(mat3(modelview) * normal);\n"
"}\n";

static const char *light_fragment_shader =
"precision mediump float;\n"
"varying vec3 vlight;\n"
"varying vec2 vtexcoord;\n"
"varying vec3 vnormal;\n"
"uniform sampler2D tex0;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  float v = max(dot(vlight, vnormal), 0.0);\n"
"  gl_FragColor = texture2D(tex0, vtexcoord) * vec4(v, v, v, 1.0) * color;\n"
"}\n";

struct showone *nemoshow_pipe_create(int type)
{
	struct showpipe *pipe;
	struct showone *one;

	pipe = (struct showpipe *)malloc(sizeof(struct showpipe));
	if (pipe == NULL)
		return NULL;
	memset(pipe, 0, sizeof(struct showpipe));

	pipe->sx = 1.0f;
	pipe->sy = 1.0f;
	pipe->sz = 1.0f;

	one = &pipe->base;
	one->type = NEMOSHOW_PIPE_TYPE;
	one->sub = type;
	one->update = nemoshow_pipe_update;
	one->destroy = nemoshow_pipe_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "light", pipe->lights, sizeof(float[4]));
	nemoobject_set_reserved(&one->object, "matrix", pipe->projection.d, sizeof(float[16]));

	nemoobject_set_reserved(&one->object, "tx", &pipe->tx, sizeof(double));
	nemoobject_set_reserved(&one->object, "ty", &pipe->ty, sizeof(double));
	nemoobject_set_reserved(&one->object, "tz", &pipe->tz, sizeof(double));
	nemoobject_set_reserved(&one->object, "sx", &pipe->sx, sizeof(double));
	nemoobject_set_reserved(&one->object, "sy", &pipe->sy, sizeof(double));
	nemoobject_set_reserved(&one->object, "sz", &pipe->sz, sizeof(double));
	nemoobject_set_reserved(&one->object, "rx", &pipe->rx, sizeof(double));
	nemoobject_set_reserved(&one->object, "ry", &pipe->ry, sizeof(double));
	nemoobject_set_reserved(&one->object, "rz", &pipe->rz, sizeof(double));

	if (one->sub == NEMOSHOW_SIMPLE_PIPE) {
		pipe->program = glshader_create_program(simple_fragment_shader, simple_vertex_shader);

		glUseProgram(pipe->program);
		glBindAttribLocation(pipe->program, 0, "vertex");
		glLinkProgram(pipe->program);

		pipe->uprojection = glGetUniformLocation(pipe->program, "projection");
		pipe->umodelview = glGetUniformLocation(pipe->program, "modelview");
		pipe->ucolor = glGetUniformLocation(pipe->program, "color");
	} else if (one->sub == NEMOSHOW_TEXTURE_PIPE) {
		pipe->program = glshader_create_program(texture_fragment_shader, texture_vertex_shader);

		glUseProgram(pipe->program);
		glBindAttribLocation(pipe->program, 0, "vertex");
		glBindAttribLocation(pipe->program, 1, "texcoord");
		glLinkProgram(pipe->program);

		pipe->uprojection = glGetUniformLocation(pipe->program, "projection");
		pipe->umodelview = glGetUniformLocation(pipe->program, "modelview");
		pipe->ucolor = glGetUniformLocation(pipe->program, "color");
		pipe->utex0 = glGetUniformLocation(pipe->program, "tex0");
	} else if (one->sub == NEMOSHOW_LIGHTING_PIPE) {
		pipe->program = glshader_create_program(light_fragment_shader, light_vertex_shader);

		glUseProgram(pipe->program);
		glBindAttribLocation(pipe->program, 0, "vertex");
		glBindAttribLocation(pipe->program, 1, "texcoord");
		glBindAttribLocation(pipe->program, 2, "normal");
		glLinkProgram(pipe->program);

		pipe->uprojection = glGetUniformLocation(pipe->program, "projection");
		pipe->umodelview = glGetUniformLocation(pipe->program, "modelview");
		pipe->ucolor = glGetUniformLocation(pipe->program, "color");
		pipe->utex0 = glGetUniformLocation(pipe->program, "tex0");
		pipe->ulight = glGetUniformLocation(pipe->program, "light");
	}

	return one;
}

void nemoshow_pipe_destroy(struct showone *one)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);

	nemoshow_one_finish(one);

	free(pipe);
}

int nemoshow_pipe_arrange(struct showone *one)
{
	return 0;
}

int nemoshow_pipe_update(struct showone *one)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);

	if ((one->dirty & NEMOSHOW_MATRIX_DIRTY) != 0) {
		nemomatrix_init_identity(&pipe->projection);
		nemomatrix_rotate_x(&pipe->projection, cos(pipe->rx * M_PI / 180.0f), sin(pipe->rx * M_PI / 180.0f));
		nemomatrix_rotate_y(&pipe->projection, cos(pipe->ry * M_PI / 180.0f), sin(pipe->ry * M_PI / 180.0f));
		nemomatrix_rotate_z(&pipe->projection, cos(pipe->rz * M_PI / 180.0f), sin(pipe->rz * M_PI / 180.0f));
		nemomatrix_scale_xyz(&pipe->projection, pipe->sx, pipe->sy * -1.0f, pipe->sz);
		nemomatrix_translate_xyz(&pipe->projection, pipe->tx, pipe->ty, pipe->tz);
	}

	nemoshow_canvas_damage_all(one->parent);

	return 0;
}

static inline int nemoshow_pipe_dispatch_simple(struct showone *canvas, struct showone *one)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);
	struct showpoly *poly;
	struct showone *child;

	glUseProgram(pipe->program);

	glUniformMatrix4fv(pipe->uprojection, 1, GL_FALSE, (GLfloat *)pipe->projection.d);

	nemoshow_children_for_each(child, one) {
		poly = NEMOSHOW_POLY(child);

		glUniformMatrix4fv(pipe->umodelview, 1, GL_FALSE, (GLfloat *)poly->modelview.d);
		glUniform4fv(pipe->ucolor, 1, poly->colors);

		if (poly->on_vbo == 0) {
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)&poly->vertices[0]);
			glEnableVertexAttribArray(0);

			glDrawArrays(poly->mode, 0, poly->elements);
		} else {
			glBindVertexArray(poly->varray);
			glDrawArrays(poly->mode, 0, poly->elements);
			glBindVertexArray(0);
		}
	}

	return 0;
}

static inline int nemoshow_pipe_dispatch_texture(struct showone *canvas, struct showone *one)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);
	struct showpoly *poly;
	struct showone *child;
	struct showone *ref;

	glUseProgram(pipe->program);

	glUniformMatrix4fv(pipe->uprojection, 1, GL_FALSE, (GLfloat *)pipe->projection.d);
	glUniform1i(pipe->utex0, 0);

	nemoshow_children_for_each(child, one) {
		poly = NEMOSHOW_POLY(child);

		glUniformMatrix4fv(pipe->umodelview, 1, GL_FALSE, (GLfloat *)poly->modelview.d);
		glUniform4fv(pipe->ucolor, 1, poly->colors);

		ref = NEMOSHOW_REF(child, NEMOSHOW_CANVAS_REF);
		if (ref != NULL)
			glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_texture(ref));

		if (poly->on_vbo == 0) {
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)&poly->vertices[0]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void *)&poly->texcoords[0]);
			glEnableVertexAttribArray(1);

			glDrawArrays(poly->mode, 0, poly->elements);
		} else {
			glBindVertexArray(poly->varray);
			glDrawArrays(poly->mode, 0, poly->elements);
			glBindVertexArray(0);
		}
	}

	return 0;
}

static inline int nemoshow_pipe_dispatch_lighting(struct showone *canvas, struct showone *one)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);
	struct showpoly *poly;
	struct showone *child;
	struct showone *ref;

	glUseProgram(pipe->program);

	glUniformMatrix4fv(pipe->uprojection, 1, GL_FALSE, (GLfloat *)pipe->projection.d);
	glUniform1i(pipe->utex0, 0);
	glUniform4fv(pipe->ulight, 1, pipe->lights);

	nemoshow_children_for_each(child, one) {
		poly = NEMOSHOW_POLY(child);

		glUniformMatrix4fv(pipe->umodelview, 1, GL_FALSE, (GLfloat *)poly->modelview.d);
		glUniform4fv(pipe->ucolor, 1, poly->colors);

		ref = NEMOSHOW_REF(child, NEMOSHOW_CANVAS_REF);
		if (ref != NULL)
			glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_texture(ref));

		if (poly->on_vbo == 0) {
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)&poly->vertices[0]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void *)&poly->texcoords[0]);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)&poly->normals[0]);
			glEnableVertexAttribArray(2);

			glDrawArrays(poly->mode, 0, poly->elements);
		} else {
			glBindVertexArray(poly->varray);
			glDrawArrays(poly->mode, 0, poly->elements);
			glBindVertexArray(0);
		}
	}

	return 0;
}

int nemoshow_pipe_dispatch_one(struct showone *canvas, struct showone *one)
{
	if (one->sub == NEMOSHOW_SIMPLE_PIPE) {
		return nemoshow_pipe_dispatch_simple(canvas, one);
	} else if (one->sub == NEMOSHOW_TEXTURE_PIPE) {
		return nemoshow_pipe_dispatch_texture(canvas, one);
	} else if (one->sub == NEMOSHOW_LIGHTING_PIPE) {
		return nemoshow_pipe_dispatch_lighting(canvas, one);
	}

	return 0;
}

void nemoshow_canvas_render_pipeline(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	struct showone *child;

	glBindFramebuffer(GL_FRAMEBUFFER, canvas->fbo);

	glViewport(0, 0, canvas->viewport.width, canvas->viewport.height);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	nemoshow_children_for_each(child, one) {
		nemoshow_pipe_dispatch_one(one, child);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
