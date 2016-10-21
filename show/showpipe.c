#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showpipe.h>
#include <showpoly.h>
#include <showcanvas.h>
#include <glshader.h>
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
"  float alpha = color.a;\n"
"  gl_FragColor.rgb = color.rgb * alpha;\n"
"  gl_FragColor.a = alpha;\n"
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
"  vec4 t = texture2D(tex0, vtexcoord);\n"
"  float alpha = t.a * color.a;\n"
"  gl_FragColor.rgb = texture2D(tex0, vtexcoord).rgb * color.rgb * alpha;\n"
"  gl_FragColor.a = alpha;\n"
"}\n";

static const char *lighting_diffuse_vertex_shader =
"uniform mat4 projection;\n"
"uniform mat4 modelview;\n"
"uniform vec4 light;\n"
"attribute vec3 vertex;\n"
"attribute vec4 diffuse;\n"
"attribute vec3 normal;\n"
"varying vec3 vlight;\n"
"varying vec4 vdiffuse;\n"
"varying vec3 vnormal;\n"
"void main() {\n"
"  gl_Position = projection * modelview * vec4(vertex, 1.0);\n"
"  vlight = normalize(light.xyz - mat3(modelview) * vertex);\n"
"  vdiffuse = diffuse;\n"
"  vnormal = normalize(mat3(modelview) * normal);\n"
"}\n";

static const char *lighting_diffuse_fragment_shader =
"precision mediump float;\n"
"varying vec3 vlight;\n"
"varying vec4 vdiffuse;\n"
"varying vec3 vnormal;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  float alpha = vdiffuse.a * color.a;\n"
"  gl_FragColor.rgb = vdiffuse.rgb * max(dot(vlight, vnormal), 0.0) * color.rgb * alpha;\n"
"  gl_FragColor.a = alpha;\n"
"}\n";

static const char *lighting_texture_vertex_shader =
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

static const char *lighting_texture_fragment_shader =
"precision mediump float;\n"
"varying vec3 vlight;\n"
"varying vec2 vtexcoord;\n"
"varying vec3 vnormal;\n"
"uniform sampler2D tex0;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  float v = max(dot(vlight, vnormal), 0.0);\n"
"  vec4 t = texture2D(tex0, vtexcoord);\n"
"  float alpha = t.a * color.a;\n"
"  gl_FragColor.rgb = t.rgb * vec3(v, v, v) * color.rgb * alpha;\n"
"  gl_FragColor.a = alpha;\n"
"}\n";

static const char *lighting_diffuse_texture_vertex_shader =
"uniform mat4 projection;\n"
"uniform mat4 modelview;\n"
"uniform vec4 light;\n"
"attribute vec3 vertex;\n"
"attribute vec2 texcoord;\n"
"attribute vec4 diffuse;\n"
"attribute vec3 normal;\n"
"varying vec3 vlight;\n"
"varying vec2 vtexcoord;\n"
"varying vec4 vdiffuse;\n"
"varying vec3 vnormal;\n"
"void main() {\n"
"  gl_Position = projection * modelview * vec4(vertex, 1.0);\n"
"  vlight = normalize(light.xyz - mat3(modelview) * vertex);\n"
"  vtexcoord = texcoord;\n"
"  vdiffuse = diffuse;\n"
"  vnormal = normalize(mat3(modelview) * normal);\n"
"}\n";

static const char *lighting_diffuse_texture_fragment_shader =
"precision mediump float;\n"
"varying vec3 vlight;\n"
"varying vec2 vtexcoord;\n"
"varying vec4 vdiffuse;\n"
"varying vec3 vnormal;\n"
"uniform sampler2D tex0;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  float v = max(dot(vlight, vnormal), 0.0);\n"
"  vec4 t = texture2D(tex0, vtexcoord);\n"
"  float alpha = t.a * vdiffuse.a * color.a;\n"
"  gl_FragColor.rgb = t.rgb * vec3(v, v, v) * vdiffuse.rgb * color.rgb * alpha;\n"
"  gl_FragColor.a = alpha;\n"
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

	pipe->ratio = 1.0f;

	one = &pipe->base;
	one->type = NEMOSHOW_PIPE_TYPE;
	one->sub = type;
	one->update = nemoshow_pipe_update;
	one->destroy = nemoshow_pipe_destroy;
	one->attach = nemoshow_pipe_attach_one;
	one->detach = nemoshow_pipe_detach_one;

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
		pipe->program = glshader_compile_program(simple_vertex_shader, simple_fragment_shader, &pipe->vshader, &pipe->fshader);

		glUseProgram(pipe->program);
		glBindAttribLocation(pipe->program, 0, "vertex");
		glLinkProgram(pipe->program);

		pipe->vertex = 0;
		pipe->texcoord = -1;
		pipe->diffuse = -1;
		pipe->normal = -1;

		pipe->uprojection = glGetUniformLocation(pipe->program, "projection");
		pipe->umodelview = glGetUniformLocation(pipe->program, "modelview");
		pipe->ucolor = glGetUniformLocation(pipe->program, "color");
	} else if (one->sub == NEMOSHOW_TEXTURE_PIPE) {
		pipe->program = glshader_compile_program(texture_vertex_shader, texture_fragment_shader, &pipe->vshader, &pipe->fshader);

		glUseProgram(pipe->program);
		glBindAttribLocation(pipe->program, 0, "vertex");
		glBindAttribLocation(pipe->program, 1, "texcoord");
		glLinkProgram(pipe->program);

		pipe->vertex = 0;
		pipe->texcoord = 1;
		pipe->diffuse = -1;
		pipe->normal = -1;

		pipe->uprojection = glGetUniformLocation(pipe->program, "projection");
		pipe->umodelview = glGetUniformLocation(pipe->program, "modelview");
		pipe->ucolor = glGetUniformLocation(pipe->program, "color");
		pipe->utex0 = glGetUniformLocation(pipe->program, "tex0");
	} else if (one->sub == NEMOSHOW_LIGHTING_DIFFUSE_PIPE) {
		pipe->program = glshader_compile_program(lighting_diffuse_vertex_shader, lighting_diffuse_fragment_shader, &pipe->vshader, &pipe->fshader);

		glUseProgram(pipe->program);
		glBindAttribLocation(pipe->program, 0, "vertex");
		glBindAttribLocation(pipe->program, 1, "diffuse");
		glBindAttribLocation(pipe->program, 2, "normal");
		glLinkProgram(pipe->program);

		pipe->vertex = 0;
		pipe->texcoord = -1;
		pipe->diffuse = 1;
		pipe->normal = 2;

		pipe->uprojection = glGetUniformLocation(pipe->program, "projection");
		pipe->umodelview = glGetUniformLocation(pipe->program, "modelview");
		pipe->ucolor = glGetUniformLocation(pipe->program, "color");
		pipe->ulight = glGetUniformLocation(pipe->program, "light");
	} else if (one->sub == NEMOSHOW_LIGHTING_TEXTURE_PIPE) {
		pipe->program = glshader_compile_program(lighting_texture_vertex_shader, lighting_texture_fragment_shader, &pipe->vshader, &pipe->fshader);

		glUseProgram(pipe->program);
		glBindAttribLocation(pipe->program, 0, "vertex");
		glBindAttribLocation(pipe->program, 1, "texcoord");
		glBindAttribLocation(pipe->program, 2, "normal");
		glLinkProgram(pipe->program);

		pipe->vertex = 0;
		pipe->texcoord = 1;
		pipe->diffuse = -1;
		pipe->normal = 2;

		pipe->uprojection = glGetUniformLocation(pipe->program, "projection");
		pipe->umodelview = glGetUniformLocation(pipe->program, "modelview");
		pipe->ucolor = glGetUniformLocation(pipe->program, "color");
		pipe->utex0 = glGetUniformLocation(pipe->program, "tex0");
		pipe->ulight = glGetUniformLocation(pipe->program, "light");
	} else if (one->sub == NEMOSHOW_LIGHTING_DIFFUSE_TEXTURE_PIPE) {
		pipe->program = glshader_compile_program(lighting_diffuse_texture_vertex_shader, lighting_diffuse_texture_fragment_shader, &pipe->vshader, &pipe->fshader);

		glUseProgram(pipe->program);
		glBindAttribLocation(pipe->program, 0, "vertex");
		glBindAttribLocation(pipe->program, 1, "texcoord");
		glBindAttribLocation(pipe->program, 2, "diffuse");
		glBindAttribLocation(pipe->program, 3, "normal");
		glLinkProgram(pipe->program);

		pipe->vertex = 0;
		pipe->texcoord = 1;
		pipe->diffuse = 2;
		pipe->normal = 3;

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

	glDeleteShader(pipe->vshader);
	glDeleteShader(pipe->fshader);
	glDeleteProgram(pipe->program);

	free(pipe);
}

void nemoshow_pipe_attach_one(struct showone *parent, struct showone *one)
{
	struct showone *canvas;

	nemoshow_one_attach_one(parent, one);

	canvas = nemoshow_one_get_parent(one, NEMOSHOW_CANVAS_TYPE, 0);
	if (canvas != NULL)
		nemoshow_canvas_set_ones(canvas, one);
}

void nemoshow_pipe_detach_one(struct showone *one)
{
	nemoshow_one_detach_one(one);

	nemoshow_canvas_put_ones(one);
}

int nemoshow_pipe_update(struct showone *one)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);

	if ((one->dirty & NEMOSHOW_MATRIX_DIRTY) != 0) {
		nemomatrix_init_identity(&pipe->projection);
		nemomatrix_rotate_x(&pipe->projection, cos(pipe->rx * M_PI / 180.0f), sin(pipe->rx * M_PI / 180.0f));
		nemomatrix_rotate_y(&pipe->projection, cos(pipe->ry * M_PI / 180.0f), sin(pipe->ry * M_PI / 180.0f));
		nemomatrix_rotate_z(&pipe->projection, cos(pipe->rz * M_PI / 180.0f), sin(pipe->rz * M_PI / 180.0f));
		nemomatrix_scale_xyz(&pipe->projection, pipe->sx, pipe->sy, pipe->sz);
		nemomatrix_translate_xyz(&pipe->projection, pipe->tx, pipe->ty, pipe->tz);
		nemomatrix_scale_xyz(&pipe->projection, pipe->ratio, -1.0f, 1.0f);
	}

	if (one->canvas != NULL)
		nemoshow_canvas_damage_all(one->canvas);

	return 0;
}

static inline void nemoshow_pipe_dispatch_simple_one(struct showpipe *pipe, struct showone *one)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);
	struct showone *child;

	if (poly->elements > 0) {
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

	nemoshow_children_for_each(child, one) {
		nemoshow_pipe_dispatch_simple_one(pipe, child);
	}
}

static int nemoshow_pipe_dispatch_simple(struct showone *canvas, struct showone *one)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);
	struct showone *child;

	glUseProgram(pipe->program);

	glUniformMatrix4fv(pipe->uprojection, 1, GL_FALSE, (GLfloat *)pipe->projection.d);

	nemoshow_children_for_each(child, one) {
		nemoshow_pipe_dispatch_simple_one(pipe, child);
	}

	return 0;
}

static inline void nemoshow_pipe_dispatch_texture_one(struct showpipe *pipe, struct showone *one)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);
	struct showone *child;
	struct showone *ref;

	if (poly->elements > 0) {
		glUniformMatrix4fv(pipe->umodelview, 1, GL_FALSE, (GLfloat *)poly->modelview.d);
		glUniform4fv(pipe->ucolor, 1, poly->colors);

		ref = NEMOSHOW_REF(one, NEMOSHOW_CANVAS_REF);
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

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	nemoshow_children_for_each(child, one) {
		nemoshow_pipe_dispatch_texture_one(pipe, child);
	}
}

static int nemoshow_pipe_dispatch_texture(struct showone *canvas, struct showone *one)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);
	struct showone *child;

	glUseProgram(pipe->program);

	glUniformMatrix4fv(pipe->uprojection, 1, GL_FALSE, (GLfloat *)pipe->projection.d);
	glUniform1i(pipe->utex0, 0);

	nemoshow_children_for_each(child, one) {
		nemoshow_pipe_dispatch_texture_one(pipe, child);
	}

	return 0;
}

static inline void nemoshow_pipe_dispatch_lighting_diffuse_one(struct showpipe *pipe, struct showone *one)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);
	struct showone *child;

	if (poly->elements > 0) {
		glUniformMatrix4fv(pipe->umodelview, 1, GL_FALSE, (GLfloat *)poly->modelview.d);
		glUniform4fv(pipe->ucolor, 1, poly->colors);

		if (poly->on_vbo == 0) {
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)&poly->vertices[0]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)&poly->diffuses[0]);
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

	nemoshow_children_for_each(child, one) {
		nemoshow_pipe_dispatch_lighting_diffuse_one(pipe, child);
	}
}

static int nemoshow_pipe_dispatch_lighting_diffuse(struct showone *canvas, struct showone *one)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);
	struct showone *child;

	glUseProgram(pipe->program);

	glUniformMatrix4fv(pipe->uprojection, 1, GL_FALSE, (GLfloat *)pipe->projection.d);
	glUniform4fv(pipe->ulight, 1, pipe->lights);

	nemoshow_children_for_each(child, one) {
		nemoshow_pipe_dispatch_lighting_diffuse_one(pipe, child);
	}

	return 0;
}

static inline void nemoshow_pipe_dispatch_lighting_texture_one(struct showpipe *pipe, struct showone *one)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);
	struct showone *child;
	struct showone *ref;

	if (poly->elements > 0) {
		glUniformMatrix4fv(pipe->umodelview, 1, GL_FALSE, (GLfloat *)poly->modelview.d);
		glUniform4fv(pipe->ucolor, 1, poly->colors);

		ref = NEMOSHOW_REF(one, NEMOSHOW_CANVAS_REF);
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

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	nemoshow_children_for_each(child, one) {
		nemoshow_pipe_dispatch_lighting_texture_one(pipe, child);
	}
}

static int nemoshow_pipe_dispatch_lighting_texture(struct showone *canvas, struct showone *one)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);
	struct showone *child;
	struct showone *ref;

	glUseProgram(pipe->program);

	glUniformMatrix4fv(pipe->uprojection, 1, GL_FALSE, (GLfloat *)pipe->projection.d);
	glUniform1i(pipe->utex0, 0);
	glUniform4fv(pipe->ulight, 1, pipe->lights);

	nemoshow_children_for_each(child, one) {
		nemoshow_pipe_dispatch_lighting_texture_one(pipe, child);
	}

	return 0;
}

static inline void nemoshow_pipe_dispatch_lighting_diffuse_texture_one(struct showpipe *pipe, struct showone *one)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);
	struct showone *child;
	struct showone *ref;

	if (poly->elements > 0) {
		glUniformMatrix4fv(pipe->umodelview, 1, GL_FALSE, (GLfloat *)poly->modelview.d);
		glUniform4fv(pipe->ucolor, 1, poly->colors);

		ref = NEMOSHOW_REF(one, NEMOSHOW_CANVAS_REF);
		if (ref != NULL)
			glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_texture(ref));

		if (poly->on_vbo == 0) {
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)&poly->vertices[0]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void *)&poly->texcoords[0]);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)&poly->diffuses[0]);
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)&poly->normals[0]);
			glEnableVertexAttribArray(3);

			glDrawArrays(poly->mode, 0, poly->elements);
		} else {
			glBindVertexArray(poly->varray);
			glDrawArrays(poly->mode, 0, poly->elements);
			glBindVertexArray(0);
		}

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	nemoshow_children_for_each(child, one) {
		nemoshow_pipe_dispatch_lighting_diffuse_texture_one(pipe, child);
	}
}

static int nemoshow_pipe_dispatch_lighting_diffuse_texture(struct showone *canvas, struct showone *one)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);
	struct showone *child;
	struct showone *ref;

	glUseProgram(pipe->program);

	glUniformMatrix4fv(pipe->uprojection, 1, GL_FALSE, (GLfloat *)pipe->projection.d);
	glUniform1i(pipe->utex0, 0);
	glUniform4fv(pipe->ulight, 1, pipe->lights);

	nemoshow_children_for_each(child, one) {
		nemoshow_pipe_dispatch_lighting_diffuse_texture_one(pipe, child);
	}

	return 0;
}

typedef int (*nemoshow_pipe_dispatch_pipeline)(struct showone *canvas, struct showone *one);

void nemoshow_canvas_render_pipeline(struct nemoshow *show, struct showone *one)
{
	static const nemoshow_pipe_dispatch_pipeline dispatches[] = {
		NULL,
		nemoshow_pipe_dispatch_simple,
		nemoshow_pipe_dispatch_texture,
		nemoshow_pipe_dispatch_lighting_diffuse,
		nemoshow_pipe_dispatch_lighting_texture,
		nemoshow_pipe_dispatch_lighting_diffuse_texture
	};
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	struct showone *child;

	glBindFramebuffer(GL_FRAMEBUFFER, canvas->fbo);

	glViewport(0, 0, canvas->viewport.width, canvas->viewport.height);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	nemoshow_children_for_each(child, one) {
		dispatches[child->sub](one, child);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
