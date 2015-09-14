#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <float.h>
#include <math.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <tiny_obj_loader.h>

#include <nemotool.h>
#include <nemoegl.h>
#include <talehelper.h>
#include <pixmanhelper.h>
#include <glhelper.h>
#include <fbohelper.h>
#include <nemomatrix.h>
#include <nemobox.h>
#include <nemomisc.h>

#define	NEMOMESH_SHADERS_MAX			(2)

struct meshcontext {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	int32_t width, height;

	GLuint fbo, dbo;

	GLuint programs[NEMOMESH_SHADERS_MAX];

	GLuint program;
	GLuint uprojection;
	GLuint ucolor;

	struct nemomatrix matrix;

	GLuint varray;
	GLuint vbuffer;
	GLuint vindex;

	uint32_t *lines;
	int nlines, slines;

	uint32_t *meshes;
	int nmeshes, smeshes;

	float *vertices;
	int nvertices, svertices;

	GLenum mode;
	int elements;

	float aspect;
	float sx, sy, sz;
	float tx, ty, tz;

	struct nemovector avec, cvec;
	struct nemoquaternion squat, cquat;
};

static const char *simple_vertex_shader =
"uniform mat4 projection;\n"
"attribute vec3 vertex;\n"
"attribute vec3 normal;\n"
"void main() {\n"
"  gl_Position = projection * vec4(vertex, 1.0f);\n"
"}\n";

static const char *simple_fragment_shader =
"precision mediump float;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  gl_FragColor = color;\n"
"}\n";

static const char *light_vertex_shader =
"uniform mat4 projection;\n"
"attribute vec3 vertex;\n"
"attribute vec3 normal;\n"
"void main() {\n"
"  gl_Position = projection * vec4(vertex, 1.0f);\n"
"}\n";

static const char *light_fragment_shader =
"precision mediump float;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  gl_FragColor = color;\n"
"}\n";

static GLuint nemomesh_create_shader(const char *fshader, const char *vshader)
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
	glBindAttribLocation(program, 1, "normal");
	glLinkProgram(program);

	return program;
}

static void nemomesh_prepare_shader(struct meshcontext *context, GLuint program)
{
	if (context->program == program)
		return;

	context->program = program;

	context->uprojection = glGetUniformLocation(context->program, "projection");
	context->ucolor = glGetUniformLocation(context->program, "color");
}

static void nemomesh_create_buffer(struct meshcontext *context, const char *filepath, const char *basepath)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string r;
	uint32_t base = 0;
	float minx = FLT_MAX, miny = FLT_MAX, minz = FLT_MAX, maxx = FLT_MIN, maxy = FLT_MIN, maxz = FLT_MIN;
	float max;
	int i, j;

	context->lines = (uint32_t *)malloc(sizeof(uint32_t) * 16);
	context->nlines = 0;
	context->slines = 16;

	context->meshes = (uint32_t *)malloc(sizeof(uint32_t) * 16);
	context->nmeshes = 0;
	context->smeshes = 16;

	context->vertices = (float *)malloc(sizeof(float) * 16);
	context->nvertices = 0;
	context->svertices = 16;

	r = tinyobj::LoadObj(shapes, materials, filepath, basepath);
	if (!r.empty())
		exit(1);

	for (i = 0; i < shapes.size(); i++) {
		for (j = 0; j < shapes[i].mesh.indices.size() / 3; j++) {
			NEMOBOX_APPEND(context->lines, context->slines, context->nlines, shapes[i].mesh.indices[j * 3 + 0] + base);
			NEMOBOX_APPEND(context->lines, context->slines, context->nlines, shapes[i].mesh.indices[j * 3 + 1] + base);
			NEMOBOX_APPEND(context->lines, context->slines, context->nlines, shapes[i].mesh.indices[j * 3 + 0] + base);
			NEMOBOX_APPEND(context->lines, context->slines, context->nlines, shapes[i].mesh.indices[j * 3 + 2] + base);
			NEMOBOX_APPEND(context->lines, context->slines, context->nlines, shapes[i].mesh.indices[j * 3 + 1] + base);
			NEMOBOX_APPEND(context->lines, context->slines, context->nlines, shapes[i].mesh.indices[j * 3 + 2] + base);

			NEMOBOX_APPEND(context->meshes, context->smeshes, context->nmeshes, shapes[i].mesh.indices[j * 3 + 0] + base);
			NEMOBOX_APPEND(context->meshes, context->smeshes, context->nmeshes, shapes[i].mesh.indices[j * 3 + 1] + base);
			NEMOBOX_APPEND(context->meshes, context->smeshes, context->nmeshes, shapes[i].mesh.indices[j * 3 + 2] + base);
		}

		if (shapes[i].mesh.normals.size() != shapes[i].mesh.positions.size()) {
			for (j = 0; j < shapes[i].mesh.positions.size() / 3; j++, base++) {
				NEMOBOX_APPEND(context->vertices, context->svertices, context->nvertices, shapes[i].mesh.positions[j * 3 + 0]);
				NEMOBOX_APPEND(context->vertices, context->svertices, context->nvertices, shapes[i].mesh.positions[j * 3 + 1]);
				NEMOBOX_APPEND(context->vertices, context->svertices, context->nvertices, shapes[i].mesh.positions[j * 3 + 2]);
				NEMOBOX_APPEND(context->vertices, context->svertices, context->nvertices, 0.0f);
				NEMOBOX_APPEND(context->vertices, context->svertices, context->nvertices, 0.0f);
				NEMOBOX_APPEND(context->vertices, context->svertices, context->nvertices, 0.0f);

				if (shapes[i].mesh.positions[j * 3 + 0] < minx)
					minx = shapes[i].mesh.positions[j * 3 + 0];
				if (shapes[i].mesh.positions[j * 3 + 1] < miny)
					miny = shapes[i].mesh.positions[j * 3 + 1];
				if (shapes[i].mesh.positions[j * 3 + 3] < minz)
					minz = shapes[i].mesh.positions[j * 3 + 2];
				if (shapes[i].mesh.positions[j * 3 + 0] > maxx)
					maxx = shapes[i].mesh.positions[j * 3 + 0];
				if (shapes[i].mesh.positions[j * 3 + 1] > maxy)
					maxy = shapes[i].mesh.positions[j * 3 + 1];
				if (shapes[i].mesh.positions[j * 3 + 2] > maxz)
					maxz = shapes[i].mesh.positions[j * 3 + 2];
			}
		} else {
			for (j = 0; j < shapes[i].mesh.positions.size() / 3; j++, base++) {
				NEMOBOX_APPEND(context->vertices, context->svertices, context->nvertices, shapes[i].mesh.positions[j * 3 + 0]);
				NEMOBOX_APPEND(context->vertices, context->svertices, context->nvertices, shapes[i].mesh.positions[j * 3 + 1]);
				NEMOBOX_APPEND(context->vertices, context->svertices, context->nvertices, shapes[i].mesh.positions[j * 3 + 2]);
				NEMOBOX_APPEND(context->vertices, context->svertices, context->nvertices, shapes[i].mesh.normals[j * 3 + 0]);
				NEMOBOX_APPEND(context->vertices, context->svertices, context->nvertices, shapes[i].mesh.normals[j * 3 + 1]);
				NEMOBOX_APPEND(context->vertices, context->svertices, context->nvertices, shapes[i].mesh.normals[j * 3 + 2]);

				if (shapes[i].mesh.positions[j * 3 + 0] < minx)
					minx = shapes[i].mesh.positions[j * 3 + 0];
				if (shapes[i].mesh.positions[j * 3 + 1] < miny)
					miny = shapes[i].mesh.positions[j * 3 + 1];
				if (shapes[i].mesh.positions[j * 3 + 3] < minz)
					minz = shapes[i].mesh.positions[j * 3 + 2];
				if (shapes[i].mesh.positions[j * 3 + 0] > maxx)
					maxx = shapes[i].mesh.positions[j * 3 + 0];
				if (shapes[i].mesh.positions[j * 3 + 1] > maxy)
					maxy = shapes[i].mesh.positions[j * 3 + 1];
				if (shapes[i].mesh.positions[j * 3 + 2] > maxz)
					maxz = shapes[i].mesh.positions[j * 3 + 2];
			}
		}
	}

	max = MAX(maxx - minx, MAX(maxy - miny, maxz - minz));

	context->sx = 2.0f / max;
	context->sy = 2.0f / max;
	context->sz = 2.0f / max;

	context->tx = -(maxx + minx) / 2.0f;
	context->ty = -(maxy + miny) / 2.0f;
	context->tz = -(maxz + minz) / 2.0f;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glGenVertexArrays(1, &context->varray);
	glBindVertexArray(context->varray);

	glGenBuffers(1, &context->vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, context->vbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)sizeof(GLfloat[3]));
	glEnableVertexAttribArray(1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * context->nvertices, context->vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &context->vindex);

	glBindVertexArray(0);
}

static void nemomesh_prepare_buffer(struct meshcontext *context, GLenum mode, uint32_t *buffers, int elements)
{
	glBindVertexArray(context->varray);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->vindex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * elements, buffers, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	context->mode = mode;
	context->elements = elements;
}

static void nemomesh_render(struct meshcontext *context)
{
	GLfloat rgba[4] = { 0.0f, 1.0f, 1.0f, 1.0f };

	glUseProgram(context->program);

	glBindFramebuffer(GL_FRAMEBUFFER, context->fbo);

	glViewport(0, 0, context->width, context->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(context->uprojection, 1, GL_FALSE, (GLfloat *)context->matrix.d);
	glUniform4fv(context->ucolor, 1, rgba);

	glBindVertexArray(context->varray);
	glDrawElements(context->mode, context->elements, GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
}

static void nemomesh_project_onto_surface(int32_t width, int32_t height, struct nemovector *v)
{
	double dist = width > height ? width : height;
	double radius = dist / 2.0f;
	double px = (v->f[0] - radius);
	double py = (v->f[1] - radius) * -1.0f;
	double pz = (v->f[2] - 0.0f);
	double radius2 = radius * radius;
	double length2 = px * px + py * py;
	double length;

	if (length2 <= radius2) {
		pz = sqrtf(radius2 - length2);
	} else {
		length = sqrtf(length2);

		px = px / length;
		py = py / length;
		pz = 0.0f;
	}

	length = sqrtf(px * px + py * py + pz * pz);

	v->f[0] = px / length;
	v->f[1] = py / length;
	v->f[2] = pz / length;
}

static void nemomesh_compute_quaternion(struct meshcontext *context)
{
	struct nemovector v = context->avec;
	float dot = nemovector_dot(&context->avec, &context->cvec);
	float angle = acosf(dot);

	if (isnan(angle) == 0) {
		nemovector_cross(&v, &context->cvec);

		nemoquaternion_make_with_angle_axis(&context->cquat, angle * 2.0f, v.f[0], v.f[1], v.f[2]);
		nemoquaternion_normalize(&context->cquat);
		nemoquaternion_multiply(&context->cquat, &context->squat);
	}
}

static void nemomesh_update_transform(struct meshcontext *context)
{
	nemomatrix_init_identity(&context->matrix);
	nemomatrix_translate_xyz(&context->matrix, context->tx, context->ty, context->tz);
	nemomatrix_multiply_quaternion(&context->matrix, &context->cquat);
	nemomatrix_scale_xyz(&context->matrix, context->sx * context->aspect, context->sy * -1.0f, context->sz * context->aspect);
}

static void nemomesh_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	struct meshcontext *context = (struct meshcontext *)nemotale_get_userdata(tale);
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (nemotale_is_touch_down(tale, event, type)) {
			nemotale_event_update_node_taps(tale, node, event, type);

			if (event->tapcount == 1) {
				nemocanvas_move(context->canvas, event->taps[0]->serial);
			} else if (event->tapcount == 2) {
				nemocanvas_pick(context->canvas,
						event->taps[0]->serial,
						event->taps[1]->serial,
						(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE));
			} else if (event->tapcount == 3) {
				nemocanvas_miss(context->canvas);

				context->avec.f[0] = event->taps[2]->x;
				context->avec.f[1] = event->taps[2]->y;
				context->avec.f[2] = 0.0f;

				nemomesh_project_onto_surface(context->width, context->height, &context->avec);

				context->squat = context->cquat;
			}
		} else if (nemotale_is_touch_motion(tale, event, type)) {
			nemotale_event_update_node_taps(tale, node, event, type);

			if (event->tapcount == 3) {
				context->cvec.f[0] = event->taps[2]->x;
				context->cvec.f[1] = event->taps[2]->y;
				context->cvec.f[2] = 0.0f;

				nemomesh_project_onto_surface(context->width, context->height, &context->cvec);
				nemomesh_compute_quaternion(context);

				nemocanvas_dispatch_frame(context->canvas);
			}
		}

		if (nemotale_is_single_click(tale, event, type)) {
			nemoquaternion_init_identity(&context->cquat);

			nemocanvas_dispatch_frame(context->canvas);
		}
	}
}

static void nemomesh_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct meshcontext *context = (struct meshcontext *)nemotale_get_userdata(tale);

	if (secs == 0 && nsecs == 0) {
		nemocanvas_feedback(canvas);
	}

	nemomesh_update_transform(context);

	nemomesh_render(context);

	nemotale_node_damage_all(context->node);

	nemotale_composite_egl(context->tale, NULL);
}

static void nemomesh_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct meshcontext *context = (struct meshcontext *)nemotale_get_userdata(tale);

	if (width == 0 || height == 0)
		return;

	if (width < 200 || height < 200)
		nemotool_exit(context->tool);

	context->width = width;
	context->height = height;

	nemotool_resize_egl_canvas(context->eglcanvas, width, height);
	nemotale_resize(context->tale, width, height);
	nemotale_node_resize_gl(context->node, width, height);
	nemotale_node_opaque(context->node, 0, 0, width, height);

	glDeleteFramebuffers(1, &context->fbo);
	glDeleteRenderbuffers(1, &context->dbo);

	fbo_prepare_context(
			nemotale_node_get_texture(context->node),
			width, height,
			&context->fbo, &context->dbo);

	nemocanvas_dispatch_frame(canvas);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "file",				required_argument,			NULL,		'f' },
		{ "path",				required_argument,			NULL,		'p' },
		{ "type",				required_argument,			NULL,		't' },
		{ "width",			required_argument,			NULL,		'w' },
		{ "height",			required_argument,			NULL,		'h' },
		{ 0 }
	};
	struct meshcontext *context;
	struct nemotool *tool;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	struct talenode *node;
	char *filepath = NULL;
	char *basepath = NULL;
	char *type = NULL;
	int32_t width = 1920;
	int32_t height = 1080;
	int opt;

	while (opt = getopt_long(argc, argv, "f:p:t:w:h:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				filepath = strdup(optarg);
				break;

			case 'p':
				basepath = strdup(optarg);
				break;

			case 't':
				type = strdup(optarg);
				break;

			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	if (filepath == NULL)
		return 0;

	context = (struct meshcontext *)malloc(sizeof(struct meshcontext));
	if (context == NULL)
		return -1;
	memset(context, 0, sizeof(struct meshcontext));

	context->width = width;
	context->height = height;
	context->aspect = (double)height / (double)width;

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	context->egl = egl = nemotool_create_egl(tool);

	context->eglcanvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_set_userdata(NTEGL_CANVAS(canvas), context);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_dispatch_resize(NTEGL_CANVAS(canvas), nemomesh_dispatch_canvas_resize);
	nemocanvas_set_dispatch_frame(NTEGL_CANVAS(canvas), nemomesh_dispatch_canvas_frame);

	context->canvas = NTEGL_CANVAS(canvas);

	context->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(canvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), nemomesh_dispatch_tale_event);
	nemotale_set_userdata(tale, context);

	context->node = node = nemotale_node_create_gl(width, height);
	nemotale_node_set_id(node, 1);
	nemotale_node_opaque(node, 0, 0, width, height);
	nemotale_attach_node(tale, node);

	nemomesh_create_buffer(context, filepath, basepath);
	nemomesh_prepare_buffer(context, GL_LINES, context->lines, context->nlines);

	context->programs[0] = nemomesh_create_shader(simple_fragment_shader, simple_vertex_shader);
	context->programs[1] = nemomesh_create_shader(light_fragment_shader, light_vertex_shader);
	nemomesh_prepare_shader(context, context->programs[0]);

	fbo_prepare_context(
			nemotale_node_get_texture(context->node),
			context->width, context->height,
			&context->fbo, &context->dbo);

	nemoquaternion_init_identity(&context->cquat);

	nemocanvas_dispatch_frame(NTEGL_CANVAS(canvas));

	nemotool_run(tool);

	glDeleteBuffers(1, &context->vbuffer);
	glDeleteBuffers(1, &context->vindex);
	glDeleteVertexArrays(1, &context->varray);

	glDeleteFramebuffers(1, &context->fbo);
	glDeleteRenderbuffers(1, &context->dbo);

	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	free(context);

	return 0;
}
