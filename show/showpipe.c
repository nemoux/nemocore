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
"varying vec2 v_texcoord;\n"
"void main() {\n"
"  gl_Position = projection * modelview * vec4(vertex, 1.0);\n"
"  v_texcoord = texcoord;\n"
"}\n";

static const char *texture_fragment_shader =
"precision mediump float;\n"
"varying vec2 v_texcoord;\n"
"uniform sampler2D tex0;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  gl_FragColor = texture2D(tex0, v_texcoord) * color;\n"
"}\n";

struct showone *nemoshow_pipe_create(int type)
{
	struct showpipe *pipe;
	struct showone *one;

	pipe = (struct showpipe *)malloc(sizeof(struct showpipe));
	if (pipe == NULL)
		return NULL;
	memset(pipe, 0, sizeof(struct showpipe));

	nemomatrix_init_identity(&pipe->projection);

	one = &pipe->base;
	one->type = NEMOSHOW_PIPE_TYPE;
	one->sub = type;
	one->update = nemoshow_pipe_update;
	one->destroy = nemoshow_pipe_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "matrix", pipe->projection.d, sizeof(float[16]));

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

		if (poly->has_vbo == 0) {
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

		if (poly->has_vbo == 0) {
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)&poly->vertices[0]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)&poly->vertices[3]);
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

int nemoshow_pipe_dispatch_one(struct showone *canvas, struct showone *one)
{
	if (one->sub == NEMOSHOW_SIMPLE_PIPE) {
		return nemoshow_pipe_dispatch_simple(canvas, one);
	} else if (one->sub == NEMOSHOW_TEXTURE_PIPE) {
		return nemoshow_pipe_dispatch_texture(canvas, one);
	}

	return 0;
}

void nemoshow_canvas_render_pipeline(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	struct showone *child;

	glBindFramebuffer(GL_FRAMEBUFFER, canvas->fbo);

	glViewport(0, 0, canvas->viewport.width, canvas->viewport.height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	nemoshow_children_for_each(child, one) {
		nemoshow_pipe_dispatch_one(one, child);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
