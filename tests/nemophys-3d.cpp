#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <BulletCollision/CollisionShapes/btBox2dShape.h>
#include <BulletCollision/CollisionShapes/btConvex2dShape.h>
#include <BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.h>
#include <BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.h>
#include <BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.h>
#include <BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.h>

#include <btBulletDynamicsCommon.h>

#include <nemoshow.h>
#include <showhelper.h>
#include <fbohelper.h>
#include <glshader.h>
#include <nemohelper.h>
#include <nemolist.h>
#include <nemolog.h>
#include <nemomisc.h>

struct physcontext {
	struct nemotool *tool;

	int width, height;
	int is_fullscreen;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;

	struct showone *canvas;

	btBroadphaseInterface *broadphase;
	btCollisionDispatcher *dispatcher;
	btConstraintSolver *solver;
	btDefaultCollisionConfiguration *configuration;

	btConvex2dConvex2dAlgorithm::CreateFunc *convex2dalgo;
	btBox2dBox2dCollisionAlgorithm::CreateFunc *box2dalgo;
	btVoronoiSimplexSolver *simplexsolver;
	btMinkowskiPenetrationDepthSolver *pdsolver;

	btDiscreteDynamicsWorld *dynamicsworld;

	struct {
		float tx, ty, tz;
		float rx, ry, rz;
		float sx, sy, sz;
	} projection;

	struct {
		float a[3], b[3], c[3], e[3];
		float near, far;
	} asymmetric;

	GLuint fbo, dbo;

	GLuint program;
	GLint uprojection;
	GLint uvtransform;
	GLint utexture;
};

static void nemophys_render_3d_one(struct physcontext *context, struct nemomatrix *projection, uint32_t texture)
{
	struct nemomatrix vtransform;
	float vertices[3 * 3] = {
		-1.0f, 1.0f, -0.5f,
		1.0f, 1.0f, -0.5f,
		-1.0f, -1.0f, -0.5f
	};
	float texcoords[2 * 3] = {
		0.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 0.0f
	};
	double secs = time_current_secs();

	nemomatrix_init_identity(&vtransform);
	nemomatrix_rotate_y(&vtransform, cos(secs * M_PI), sin(secs * M_PI));

	glUseProgram(context->program);
	glBindAttribLocation(context->program, 0, "position");
	glBindAttribLocation(context->program, 1, "texcoord");

	glUniform1i(context->utexture, 0);
	glUniformMatrix4fv(context->uprojection, 1, GL_FALSE, projection->d);
	glUniformMatrix4fv(context->uvtransform, 1, GL_FALSE, vtransform.d);

	glBindTexture(GL_TEXTURE_2D, texture);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), &texcoords[0]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glBindTexture(GL_TEXTURE_2D, 0);
}

static void nemophys_dispatch_canvas_redraw(struct nemoshow *show, struct showone *canvas)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);
	struct nemomatrix projection;

	nemomatrix_init_identity(&projection);
	nemomatrix_scale_xyz(&projection, context->projection.sx, context->projection.sy, context->projection.sz);
	nemomatrix_rotate_x(&projection, cos(context->projection.rx), sin(context->projection.rx));
	nemomatrix_rotate_y(&projection, cos(context->projection.ry), sin(context->projection.ry));
	nemomatrix_rotate_z(&projection, cos(context->projection.rz), sin(context->projection.rz));
	nemomatrix_translate_xyz(&projection, context->projection.tx, context->projection.ty, context->projection.tz);
	nemomatrix_asymmetric(&projection, context->asymmetric.a, context->asymmetric.b, context->asymmetric.c, context->asymmetric.e, context->asymmetric.near, context->asymmetric.far);

	glBindFramebuffer(GL_FRAMEBUFFER, context->fbo);

	glViewport(0, 0,
			nemoshow_canvas_get_viewport_width(canvas),
			nemoshow_canvas_get_viewport_height(canvas));

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	nemophys_render_3d_one(context, &projection, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	nemoshow_one_dirty(canvas, NEMOSHOW_REDRAW_DIRTY);
}

static void nemophys_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);
	float x = nemoshow_event_get_x(event) * nemoshow_canvas_get_viewport_sx(canvas);
	float y = nemoshow_event_get_y(event) * nemoshow_canvas_get_viewport_sy(canvas);

	nemoshow_event_update_taps(show, canvas, event);

	if (context->is_fullscreen == 0) {
		if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
			if (nemoshow_event_is_more_taps(show, event, 3)) {
				nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ROTATE_TYPE | NEMOSHOW_VIEW_PICK_TRANSLATE_TYPE);

				nemoshow_event_set_cancel(event);

				nemoshow_dispatch_grab_all(show, event);
			}
		}
	}
}

static void nemophys_enter_show_frame(struct nemoshow *show, uint32_t msecs)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);

	context->dynamicsworld->stepSimulation(1.0f / 60.0f, 0);
}

static void nemophys_leave_show_frame(struct nemoshow *show, uint32_t msecs)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);
}

static void nemophys_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);

	nemoshow_view_resize(context->show, width, height);

	context->width = width;
	context->height = height;

	nemoshow_view_redraw(context->show);
}

static void nemophys_dispatch_show_fullscreen(struct nemoshow *show, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);

	if (id == NULL)
		context->is_fullscreen = 0;
	else
		context->is_fullscreen = 1;
}

static int nemophys_prepare_bullet(struct physcontext *context)
{
	context->configuration = new btDefaultCollisionConfiguration();
	context->dispatcher = new btCollisionDispatcher(context->configuration);
	context->simplexsolver = new btVoronoiSimplexSolver();
	context->pdsolver = new btMinkowskiPenetrationDepthSolver();

	context->convex2dalgo = new btConvex2dConvex2dAlgorithm::CreateFunc(context->simplexsolver, context->pdsolver);
	context->box2dalgo = new btBox2dBox2dCollisionAlgorithm::CreateFunc();

	context->dispatcher->registerCollisionCreateFunc(CONVEX_2D_SHAPE_PROXYTYPE, CONVEX_2D_SHAPE_PROXYTYPE, context->convex2dalgo);
	context->dispatcher->registerCollisionCreateFunc(BOX_2D_SHAPE_PROXYTYPE, CONVEX_2D_SHAPE_PROXYTYPE, context->convex2dalgo);
	context->dispatcher->registerCollisionCreateFunc(CONVEX_2D_SHAPE_PROXYTYPE, BOX_2D_SHAPE_PROXYTYPE, context->convex2dalgo);
	context->dispatcher->registerCollisionCreateFunc(BOX_2D_SHAPE_PROXYTYPE, BOX_2D_SHAPE_PROXYTYPE, context->box2dalgo);

	context->broadphase = new btDbvtBroadphase();
	context->solver = new btSequentialImpulseConstraintSolver;

	context->dynamicsworld = new btDiscreteDynamicsWorld(context->dispatcher, context->broadphase, context->solver, context->configuration);
	context->dynamicsworld->setGravity(btVector3(0, 300, 0));

	return 0;
}

static void nemophys_finish_bullet(struct physcontext *context)
{
	delete context->dynamicsworld;

	delete context->solver;

	delete context->broadphase;
	delete context->dispatcher;

	delete context->configuration;

	delete context->convex2dalgo;
	delete context->box2dalgo;

	delete context->simplexsolver;
	delete context->pdsolver;
}

static int nemophys_prepare_opengl(struct physcontext *context, int32_t width, int32_t height)
{
	static const char *vertexshader_texture =
		"uniform mat4 projection;\n"
		"uniform mat4 vtransform;\n"
		"attribute vec3 position;\n"
		"attribute vec2 texcoord;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = projection * vtransform * vec4(position.xyz, 1.0);\n"
		"  vtexcoord = texcoord;\n"
		"}\n";
	static const char *fragmentshader_texture =
		"precision mediump float;\n"
		"uniform sampler2D texture;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(texture, vtexcoord) + vec4(0.0, 1.0, 1.0, 1.0);\n"
		"}\n";

	fbo_prepare_context(
			nemoshow_canvas_get_texture(context->canvas),
			width, height,
			&context->fbo, &context->dbo);

	context->program = glshader_compile_program(vertexshader_texture, fragmentshader_texture, NULL, NULL);

	context->uprojection = glGetUniformLocation(context->program, "projection");
	context->uvtransform = glGetUniformLocation(context->program, "vtransform");
	context->utexture = glGetUniformLocation(context->program, "texture");

	return 0;
}

static void nemophys_finish_opengl(struct physcontext *context)
{
	glDeleteFramebuffers(1, &context->fbo);
	glDeleteRenderbuffers(1, &context->dbo);

	glDeleteProgram(context->program);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "fullscreen",			required_argument,			NULL,		'f' },
		{ 0 }
	};

	struct physcontext *context;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct showtransition *trans;
	char *fullscreen = NULL;
	int width = 800;
	int height = 800;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "f:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				fullscreen = strdup(optarg);
				break;

			default:
				break;
		}
	}

	context = (struct physcontext *)malloc(sizeof(struct physcontext));
	if (context == NULL)
		goto err1;
	memset(context, 0, sizeof(struct physcontext));

	context->width = width;
	context->height = height;

	context->projection.tx = 0.0f;
	context->projection.ty = 0.0f;
	context->projection.tz = 0.0f;
	context->projection.rx = 0.0f;
	context->projection.ry = 0.0f;
	context->projection.rz = 0.0f;
	context->projection.sx = 1.0f;
	context->projection.sy = 1.0f;
	context->projection.sz = 1.0f;

	context->asymmetric.a[0] = -1.0f;
	context->asymmetric.a[1] = -1.0f;
	context->asymmetric.a[2] = 0.0f;
	context->asymmetric.b[0] = 1.0f;
	context->asymmetric.b[1] = -1.0f;
	context->asymmetric.b[2] = 0.0f;
	context->asymmetric.c[0] = -1.0f;
	context->asymmetric.c[1] = 1.0f;
	context->asymmetric.c[2] = 0.0f;
	context->asymmetric.e[0] = 0.0f;
	context->asymmetric.e[1] = 0.0f;
	context->asymmetric.e[2] = 1.0f;
	context->asymmetric.near = 0.00001f;
	context->asymmetric.far = 10.0f;

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err2;
	nemotool_connect_wayland(tool, NULL);

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err3;
	nemoshow_set_enter_frame(show, nemophys_enter_show_frame);
	nemoshow_set_leave_frame(show, nemophys_leave_show_frame);
	nemoshow_set_dispatch_resize(show, nemophys_dispatch_show_resize);
	nemoshow_set_dispatch_fullscreen(show, nemophys_dispatch_show_fullscreen);
	nemoshow_set_userdata(show, context);

	if (fullscreen != NULL)
		nemoshow_view_set_fullscreen(show, fullscreen);

	context->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	context->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 1.0f);
	nemoshow_one_attach(scene, canvas);

	context->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_redraw(canvas, nemophys_dispatch_canvas_redraw);
	nemoshow_canvas_set_dispatch_event(canvas, nemophys_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	nemophys_prepare_opengl(context, width, height);
	nemophys_prepare_bullet(context);

	trans = nemoshow_transition_create(NEMOSHOW_LINEAR_EASE, 18000, 0);
	nemoshow_transition_dirty_one(trans, context->canvas, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_attach_transition(show, trans);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemophys_finish_bullet(context);
	nemophys_finish_opengl(context);

	nemoshow_destroy_view(show);

err3:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err2:
	free(context);

err1:
	return 0;
}
