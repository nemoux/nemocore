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

#include <nemotool.h>
#include <nemoegl.h>
#include <talehelper.h>
#include <pixmanhelper.h>
#include <glhelper.h>
#include <fbohelper.h>
#include <oshelper.h>
#include <meshhelper.h>
#include <nemomatrix.h>
#include <nemometro.h>
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
	float aspect;

	GLuint fbo, dbo;

	GLuint programs[NEMOMESH_SHADERS_MAX];

	GLuint program;
	GLuint umvp;
	GLuint uprojection;
	GLuint umodelview;
	GLuint ulight;
	GLuint ucolor;

	struct nemomatrix projection;

	struct nemomesh *mesh;
};

static const char *simple_vertex_shader =
"uniform mat4 mvp;\n"
"uniform mat4 projection;\n"
"uniform mat4 modelview;\n"
"attribute vec3 vertex;\n"
"attribute vec3 normal;\n"
"attribute vec3 diffuse;\n"
"void main() {\n"
"  gl_Position = mvp * vec4(vertex, 1.0);\n"
"}\n";

static const char *simple_fragment_shader =
"precision mediump float;\n"
"uniform vec4 light;\n"
"uniform vec4 color;\n"
"void main() {\n"
"  gl_FragColor = color;\n"
"}\n";

static const char *light_vertex_shader =
"uniform mat4 mvp;\n"
"uniform mat4 projection;\n"
"uniform mat4 modelview;\n"
"uniform vec4 light;\n"
"attribute vec3 vertex;\n"
"attribute vec3 normal;\n"
"attribute vec3 diffuse;\n"
"varying vec3 vnormal;\n"
"varying vec3 vlight;\n"
"varying vec3 vdiffuse;\n"
"void main() {\n"
"  gl_Position = mvp * vec4(vertex, 1.0);\n"
"  vlight = normalize(light.xyz - mat3(modelview) * vertex);\n"
"  vnormal = normalize(mat3(modelview) * normal);\n"
"  vdiffuse = diffuse;\n"
"}\n";

static const char *light_fragment_shader =
"precision mediump float;\n"
"uniform vec4 color;\n"
"varying vec3 vlight;\n"
"varying vec3 vnormal;\n"
"varying vec3 vdiffuse;\n"
"void main() {\n"
"  gl_FragColor = vec4(vdiffuse * max(dot(vlight, vnormal), 0.0), 1.0) * color;\n"
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
	glBindAttribLocation(program, 2, "diffuse");
	glLinkProgram(program);

	return program;
}

static void nemomesh_prepare_shader(struct meshcontext *context, GLuint program)
{
	if (context->program == program)
		return;

	context->program = program;

	context->umvp = glGetUniformLocation(context->program, "mvp");
	context->uprojection = glGetUniformLocation(context->program, "projection");
	context->umodelview = glGetUniformLocation(context->program, "modelview");
	context->ulight = glGetUniformLocation(context->program, "light");
	context->ucolor = glGetUniformLocation(context->program, "color");
}

static void nemomesh_render_object(struct meshcontext *context, struct nemomesh *mesh)
{
	struct nemomatrix matrix;

	matrix = mesh->modelview;
	nemomatrix_multiply(&matrix, &context->projection);

	glUniformMatrix4fv(context->umvp, 1, GL_FALSE, (GLfloat *)matrix.d);
	glUniformMatrix4fv(context->umodelview, 1, GL_FALSE, (GLfloat *)mesh->modelview.d);

	glBindVertexArray(mesh->varray);
	glDrawArrays(mesh->mode, 0, mesh->elements);
	glBindVertexArray(0);

	if (mesh->on_guides != 0) {
		glPointSize(10.0f);

		glBindVertexArray(mesh->garray);
		glDrawArrays(GL_LINES, 0, mesh->nguides);
		glBindVertexArray(0);
	}
}

static void nemomesh_render_scene(struct meshcontext *context)
{
	struct nemovector light = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	glUseProgram(context->program);

	glBindFramebuffer(GL_FRAMEBUFFER, context->fbo);

	glViewport(0, 0, context->width, context->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(context->uprojection, 1, GL_FALSE, (GLfloat *)context->projection.d);
	glUniform4fv(context->ulight, 1, light.f);
	glUniform4fv(context->ucolor, 1, color);

	nemomesh_render_object(context, context->mesh);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
}

static void nemomesh_unproject(struct meshcontext *context, struct nemomesh *mesh, float x, float y, float z, float *out)
{
	struct nemomatrix matrix;
	struct nemomatrix inverse;
	struct nemovector in;

	matrix = mesh->modelview;
	nemomatrix_multiply(&matrix, &context->projection);

	nemomatrix_invert(&inverse, &matrix);

	in.f[0] = x * 2.0f / context->width - 1.0f;
	in.f[1] = y * 2.0f / context->height - 1.0f;
	in.f[2] = z * 2.0f - 1.0f;
	in.f[3] = 1.0f;

	nemomatrix_transform(&inverse, &in);

	if (fabsf(in.f[3]) < 1e-6) {
		out[0] = 0.0f;
		out[1] = 0.0f;
		out[2] = 0.0f;
	} else {
		out[0] = in.f[0] / in.f[3];
		out[1] = in.f[1] / in.f[3];
		out[2] = in.f[2] / in.f[3];
	}
}

static int nemomesh_pick_object(struct meshcontext *context, struct nemomesh *mesh, float x, float y)
{
	float near[3], far[3];
	float rayorg[3];
	float rayvec[3];
	float raylen;
	float mint, maxt;

	nemomesh_unproject(context, mesh, x, y, -1.0f, near);
	nemomesh_unproject(context, mesh, x, y, 1.0f, far);

	rayvec[0] = far[0] - near[0];
	rayvec[1] = far[1] - near[1];
	rayvec[2] = far[2] - near[2];

	raylen = sqrtf(rayvec[0] * rayvec[0] + rayvec[1] * rayvec[1] + rayvec[2] * rayvec[2]);

	rayvec[0] /= raylen;
	rayvec[1] /= raylen;
	rayvec[2] /= raylen;

	rayorg[0] = near[0];
	rayorg[1] = near[1];
	rayorg[2] = near[2];

	return nemometro_cube_intersect(mesh->boundingbox, rayorg, rayvec, &mint, &maxt);
}

static void nemomesh_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	struct meshcontext *context = (struct meshcontext *)nemotale_get_userdata(tale);
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (nemotale_is_touch_down(tale, event, type)) {
			struct nemomesh *mesh = context->mesh;
			int plane = nemomesh_pick_object(context, mesh, event->x, event->y);

			if (plane == NEMO_METRO_NONE_PLANE) {
				float sx, sy;

				nemotale_event_transform_to_viewport(tale, event->x, event->y, &sx, &sy);
				nemotool_bypass_touch(context->tool, event->device, sx, sy);
			} else {
				nemotale_event_update_node_taps(tale, node, event, type);

				if (nemotale_is_single_tap(tale, event, type)) {
					nemocanvas_move(context->canvas, event->taps[0]->serial);
				} else if (nemotale_is_double_taps(tale, event, type)) {
					nemocanvas_pick(context->canvas,
							event->taps[0]->serial,
							event->taps[1]->serial,
							(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE));

					nemomesh_turn_on_guides(mesh);

					nemocanvas_dispatch_frame(context->canvas);
				} else if (nemotale_is_triple_taps(tale, event, type)) {
					nemomesh_reset_quaternion(mesh,
							context->width,
							context->height,
							event->taps[2]->x,
							event->taps[2]->y);
				}
			}
		} else if (nemotale_is_touch_motion(tale, event, type)) {
			struct nemomesh *mesh = context->mesh;

			nemotale_event_update_node_taps(tale, node, event, type);

			if (nemotale_is_triple_taps(tale, event, type)) {
				nemomesh_update_quaternion(mesh,
						context->width,
						context->height,
						event->taps[2]->x,
						event->taps[2]->y);

				nemocanvas_dispatch_frame(context->canvas);
			}
		} else if (nemotale_is_touch_up(tale, event, type)) {
			struct nemomesh *mesh = context->mesh;

			nemotale_event_update_node_taps(tale, node, event, type);

			if (nemotale_is_double_taps(tale, event, type)) {
				nemomesh_turn_off_guides(mesh);

				nemocanvas_dispatch_frame(context->canvas);
			}
		}

		if (nemotale_is_single_click(tale, event, type)) {
			struct nemomesh *mesh = context->mesh;
			int plane = nemomesh_pick_object(context, mesh, event->x, event->y);

			if (plane != NEMO_METRO_NONE_PLANE) {
				if (mesh->mode == GL_TRIANGLES)
					nemomesh_prepare_buffer(mesh, GL_LINES, mesh->lines, mesh->nlines);
				else
					nemomesh_prepare_buffer(mesh, GL_TRIANGLES, mesh->meshes, mesh->nmeshes);

				nemocanvas_dispatch_frame(context->canvas);
			}
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

	nemomesh_update_transform(context->mesh);

	nemomesh_render_scene(context);

	nemotale_node_damage_all(context->node);

	nemotale_composite_egl(context->tale, NULL);
}

static void nemomesh_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height, int32_t fixed)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct meshcontext *context = (struct meshcontext *)nemotale_get_userdata(tale);

	if (width == 0 || height == 0)
		return;

	if (width < nemotale_get_minimum_width(tale) || height < nemotale_get_minimum_height(tale)) {
		nemotool_exit(context->tool);
		return;
	}

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
	struct nemomesh *mesh;
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

	nemomatrix_init_identity(&context->projection);
	nemomatrix_scale_xyz(&context->projection, context->aspect, -1.0f, context->aspect * -1.0f);

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	context->egl = egl = nemotool_create_egl(tool);

	context->eglcanvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_max_size(NTEGL_CANVAS(canvas), width * 4, height * 4);
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

	context->programs[0] = nemomesh_create_shader(simple_fragment_shader, simple_vertex_shader);
	context->programs[1] = nemomesh_create_shader(light_fragment_shader, light_vertex_shader);
	nemomesh_prepare_shader(context, context->programs[1]);

	fbo_prepare_context(
			nemotale_node_get_texture(context->node),
			context->width, context->height,
			&context->fbo, &context->dbo);

	if (basepath == NULL)
		basepath = os_get_file_path(filepath);

	context->mesh = mesh = nemomesh_create_object(filepath, basepath);
	nemomesh_prepare_buffer(mesh, GL_LINES, mesh->lines, mesh->nlines);

	nemocanvas_dispatch_frame(NTEGL_CANVAS(canvas));

	nemotool_run(tool);

	nemomesh_destroy_object(context->mesh);

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
