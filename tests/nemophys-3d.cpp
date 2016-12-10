#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletDynamics/ConstraintSolver/btPoint2PointConstraint.h>

#include <nemoshow.h>
#include <showhelper.h>
#include <glhelper.h>
#include <nemoplay.h>
#include <playback.h>
#include <nemofs.h>
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

	btSoftBodyWorldInfo *worldinfo;
	btCollisionDispatcher *dispatcher;
	btDefaultCollisionConfiguration *configuration;
	btBroadphaseInterface *broadphase;
	btConstraintSolver *solver;
	btSoftRigidDynamicsWorld *dynamicsworld;

	btSoftBody *softbody;
	float *vertices;
	float *texcoords;
	int nvertices;

	struct {
		float tx, ty, tz;
		float rx, ry, rz;
		float sx, sy, sz;
	} projection;

	struct {
		float left, right;
		float bottom, top;
		float near, far;
	} perspective;

	GLuint fbo, dbo;

	GLuint programs[2];
	GLint uprojection0;
	GLint uvtransform0;
	GLint utexture0;
	GLint uprojection1;
	GLint uvtransform1;
	GLint ucolor1;

	struct fsdir *movies;
	int imovies;

	struct showone *video;
	struct nemoplay *play;
	struct playback_decoder *decoderback;
	struct playback_audio *audioback;
	struct playback_video *videoback;

	struct nemolist obj_list;
	int ishapes;
};

struct objone {
	struct nemolist link;

	float *vertices;
	float *texcoords;
	float *normals;
	int nvertices;

	float color[4];

	float sx, sy, sz;

	btRigidBody *body;

	struct showone *canvas;
};

static void nemophys_one_load_cube(struct objone *one)
{
	static const float vertices[] = {
		1.0f, 1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,

		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, 1.0f,

		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,

		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f,

		-1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f
	};
	static const float texcoords[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,

		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,

		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,

		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,

		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,

		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	static const float normals[] = {
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,

		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,

		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,

		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f
	};

	one->nvertices = 6 * 6;

	one->vertices = (float *)malloc(sizeof(float[3]) * 6 * 6);
	one->texcoords = (float *)malloc(sizeof(float[2]) * 6 * 6);
	one->normals = (float *)malloc(sizeof(float[3]) * 6 * 6);

	memcpy(one->vertices, vertices, sizeof(float[3]) * 6 * 6);
	memcpy(one->texcoords, texcoords, sizeof(float[2]) * 6 * 6);
	memcpy(one->normals, normals, sizeof(float[3]) * 6 * 6);
}

static void nemophys_one_load_sphere(struct objone *one, int rings, int sectors, float radius)
{
	float vr = 1.0f / (float)(rings - 0);
	float vs = 1.0f / (float)(sectors - 0);
	float tr = 1.0f / (float)(rings - 0);
	float ts = 1.0f / (float)(sectors - 0);
	int r, s;
	int i;

	one->nvertices = rings * sectors * 6;

	one->vertices = (float *)malloc(sizeof(float[3]) * rings * sectors * 6);
	one->texcoords = (float *)malloc(sizeof(float[2]) * rings * sectors * 6);
	one->normals = (float *)malloc(sizeof(float[3]) * rings * sectors * 6);

#define NEMOSPHERE_X(s, vs, r, vr)		(cos(2 * M_PI * (s) * vs) * sin(M_PI * (r) * vr))
#define NEMOSPHERE_Y(s, vs, r, vr)		(sin(-M_PI_2 + M_PI * (r) * vr))
#define NEMOSPHERE_Z(s, vs, r, vr)		(sin(2 * M_PI * (s) * vs) * sin(M_PI * (r) * vr))

	for (r = 0, i = 0; r < rings; r++) {
		for (s = 0; s < sectors; s++, i++) {
			one->vertices[(i * 6 + 0) * 3 + 0] = NEMOSPHERE_X(s + 0, vs, r + 0, vr) * radius;
			one->vertices[(i * 6 + 0) * 3 + 1] = NEMOSPHERE_Y(s + 0, vs, r + 0, vr) * radius;
			one->vertices[(i * 6 + 0) * 3 + 2] = NEMOSPHERE_Z(s + 0, vs, r + 0, vr) * radius;
			one->texcoords[(i * 6 + 0) * 2 + 0] = (s + 0) * ts;
			one->texcoords[(i * 6 + 0) * 2 + 1] = (r + 0) * tr;
			one->normals[(i * 6 + 0) * 3 + 0] = NEMOSPHERE_X(s + 0, vs, r + 0, vr);
			one->normals[(i * 6 + 0) * 3 + 1] = NEMOSPHERE_Y(s + 0, vs, r + 0, vr);
			one->normals[(i * 6 + 0) * 3 + 2] = NEMOSPHERE_Z(s + 0, vs, r + 0, vr);
			one->vertices[(i * 6 + 1) * 3 + 0] = NEMOSPHERE_X(s + 1, vs, r + 0, vr) * radius;
			one->vertices[(i * 6 + 1) * 3 + 1] = NEMOSPHERE_Y(s + 1, vs, r + 0, vr) * radius;
			one->vertices[(i * 6 + 1) * 3 + 2] = NEMOSPHERE_Z(s + 1, vs, r + 0, vr) * radius;
			one->texcoords[(i * 6 + 1) * 2 + 0] = (s + 1) * ts;
			one->texcoords[(i * 6 + 1) * 2 + 1] = (r + 0) * tr;
			one->normals[(i * 6 + 1) * 3 + 0] = NEMOSPHERE_X(s + 1, vs, r + 0, vr);
			one->normals[(i * 6 + 1) * 3 + 1] = NEMOSPHERE_Y(s + 1, vs, r + 0, vr);
			one->normals[(i * 6 + 1) * 3 + 2] = NEMOSPHERE_Z(s + 1, vs, r + 0, vr);
			one->vertices[(i * 6 + 2) * 3 + 0] = NEMOSPHERE_X(s + 1, vs, r + 1, vr) * radius;
			one->vertices[(i * 6 + 2) * 3 + 1] = NEMOSPHERE_Y(s + 1, vs, r + 1, vr) * radius;
			one->vertices[(i * 6 + 2) * 3 + 2] = NEMOSPHERE_Z(s + 1, vs, r + 1, vr) * radius;
			one->texcoords[(i * 6 + 2) * 2 + 0] = (s + 1) * ts;
			one->texcoords[(i * 6 + 2) * 2 + 1] = (r + 1) * tr;
			one->normals[(i * 6 + 2) * 3 + 0] = NEMOSPHERE_X(s + 1, vs, r + 1, vr);
			one->normals[(i * 6 + 2) * 3 + 1] = NEMOSPHERE_Y(s + 1, vs, r + 1, vr);
			one->normals[(i * 6 + 2) * 3 + 2] = NEMOSPHERE_Z(s + 1, vs, r + 1, vr);

			one->vertices[(i * 6 + 3) * 3 + 0] = NEMOSPHERE_X(s + 0, vs, r + 0, vr) * radius;
			one->vertices[(i * 6 + 3) * 3 + 1] = NEMOSPHERE_Y(s + 0, vs, r + 0, vr) * radius;
			one->vertices[(i * 6 + 3) * 3 + 2] = NEMOSPHERE_Z(s + 0, vs, r + 0, vr) * radius;
			one->texcoords[(i * 6 + 3) * 2 + 0] = (s + 0) * ts;
			one->texcoords[(i * 6 + 3) * 2 + 1] = (r + 0) * tr;
			one->normals[(i * 6 + 3) * 3 + 0] = NEMOSPHERE_X(s + 0, vs, r + 0, vr);
			one->normals[(i * 6 + 3) * 3 + 1] = NEMOSPHERE_Y(s + 0, vs, r + 0, vr);
			one->normals[(i * 6 + 3) * 3 + 2] = NEMOSPHERE_Z(s + 0, vs, r + 0, vr);
			one->vertices[(i * 6 + 4) * 3 + 0] = NEMOSPHERE_X(s + 1, vs, r + 1, vr) * radius;
			one->vertices[(i * 6 + 4) * 3 + 1] = NEMOSPHERE_Y(s + 1, vs, r + 1, vr) * radius;
			one->vertices[(i * 6 + 4) * 3 + 2] = NEMOSPHERE_Z(s + 1, vs, r + 1, vr) * radius;
			one->texcoords[(i * 6 + 4) * 2 + 0] = (s + 1) * ts;
			one->texcoords[(i * 6 + 4) * 2 + 1] = (r + 1) * tr;
			one->normals[(i * 6 + 4) * 3 + 0] = NEMOSPHERE_X(s + 1, vs, r + 1, vr);
			one->normals[(i * 6 + 4) * 3 + 1] = NEMOSPHERE_Y(s + 1, vs, r + 1, vr);
			one->normals[(i * 6 + 4) * 3 + 2] = NEMOSPHERE_Z(s + 1, vs, r + 1, vr);
			one->vertices[(i * 6 + 5) * 3 + 0] = NEMOSPHERE_X(s + 0, vs, r + 1, vr) * radius;
			one->vertices[(i * 6 + 5) * 3 + 1] = NEMOSPHERE_Y(s + 0, vs, r + 1, vr) * radius;
			one->vertices[(i * 6 + 5) * 3 + 2] = NEMOSPHERE_Z(s + 0, vs, r + 1, vr) * radius;
			one->texcoords[(i * 6 + 5) * 2 + 0] = (s + 0) * ts;
			one->texcoords[(i * 6 + 5) * 2 + 1] = (r + 1) * tr;
			one->normals[(i * 6 + 5) * 3 + 0] = NEMOSPHERE_X(s + 0, vs, r + 1, vr);
			one->normals[(i * 6 + 5) * 3 + 1] = NEMOSPHERE_Y(s + 0, vs, r + 1, vr);
			one->normals[(i * 6 + 5) * 3 + 2] = NEMOSPHERE_Z(s + 0, vs, r + 1, vr);
		}
	}
}

static void nemophys_one_set_body(struct objone *one, btRigidBody *body)
{
	one->body = body;
}

static void nemophys_one_set_canvas(struct objone *one, struct showone *canvas)
{
	one->canvas = canvas;
}

static void nemophys_one_set_color(struct objone *one, float r, float g, float b, float a)
{
	one->color[0] = r;
	one->color[1] = g;
	one->color[2] = b;
	one->color[3] = a;
}

static void nemophys_one_set_scale(struct objone *one, float sx, float sy, float sz)
{
	one->sx = sx;
	one->sy = sy;
	one->sz = sz;
}

static struct objone *nemophys_one_create(struct physcontext *context)
{
	struct objone *one;

	one = (struct objone *)malloc(sizeof(struct objone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct objone));

	nemolist_insert_tail(&context->obj_list, &one->link);

	return one;
}

static void nemophys_one_destroy(struct physcontext *context, struct objone *one)
{
	nemolist_remove(&one->link);

	free(one->vertices);
	free(one->texcoords);
	free(one->normals);

	free(one);
}

static void nemophys_render_3d_one(struct physcontext *context, struct nemomatrix *projection, struct objone *one)
{
	struct nemomatrix vtransform;

	btTransform transform;
	one->body->getMotionState()->getWorldTransform(transform);

	btQuaternion quaternion;
	quaternion = transform.getRotation();

	nemomatrix_init_identity(&vtransform);
	nemomatrix_scale_xyz(&vtransform, one->sx, one->sy, one->sz);
	nemomatrix_multiply_quaternion(&vtransform,
			quaternion.x(),
			quaternion.y(),
			quaternion.z(),
			quaternion.w());
	nemomatrix_translate_xyz(&vtransform,
			transform.getOrigin().getX(),
			transform.getOrigin().getY(),
			transform.getOrigin().getZ());

	if (one->canvas == NULL) {
		glUseProgram(context->programs[1]);
		glBindAttribLocation(context->programs[1], 0, "position");

		glUniform4fv(context->ucolor1, 1, one->color);
		glUniformMatrix4fv(context->uprojection1, 1, GL_FALSE, projection->d);
		glUniformMatrix4fv(context->uvtransform1, 1, GL_FALSE, vtransform.d);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &one->vertices[0]);
		glEnableVertexAttribArray(0);

		glDrawArrays(GL_TRIANGLES, 0, one->nvertices);
	} else {
		glUseProgram(context->programs[0]);
		glBindAttribLocation(context->programs[0], 0, "position");
		glBindAttribLocation(context->programs[0], 1, "texcoord");

		glUniform4fv(context->utexture0, 1, 0);
		glUniformMatrix4fv(context->uprojection0, 1, GL_FALSE, projection->d);
		glUniformMatrix4fv(context->uvtransform0, 1, GL_FALSE, vtransform.d);

		glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_effective_texture(one->canvas));

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &one->vertices[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), &one->texcoords[0]);
		glEnableVertexAttribArray(1);

		glDrawArrays(GL_TRIANGLES, 0, one->nvertices);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

static void nemophys_render_3d_softbody(struct physcontext *context, struct nemomatrix *projection, uint32_t texture)
{
	struct nemomatrix vtransform;

	nemomatrix_init_identity(&vtransform);

	glUseProgram(context->programs[0]);
	glBindAttribLocation(context->programs[0], 0, "position");
	glBindAttribLocation(context->programs[0], 1, "texcoord");

	glUniform1i(context->utexture0, 0);
	glUniformMatrix4fv(context->uprojection0, 1, GL_FALSE, projection->d);
	glUniformMatrix4fv(context->uvtransform0, 1, GL_FALSE, vtransform.d);

	glBindTexture(GL_TEXTURE_2D, texture);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &context->vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), &context->texcoords[0]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLES, 0, context->nvertices);

	glBindTexture(GL_TEXTURE_2D, 0);
}

static void nemophys_dispatch_canvas_redraw(struct nemoshow *show, struct showone *canvas)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);
	struct nemomatrix projection;
	struct objone *one;

	nemomatrix_init_identity(&projection);
	nemomatrix_scale_xyz(&projection, context->projection.sx, context->projection.sy, context->projection.sz);
	nemomatrix_rotate_x(&projection, cos(context->projection.rx), sin(context->projection.rx));
	nemomatrix_rotate_y(&projection, cos(context->projection.ry), sin(context->projection.ry));
	nemomatrix_rotate_z(&projection, cos(context->projection.rz), sin(context->projection.rz));
	nemomatrix_translate_xyz(&projection, context->projection.tx, context->projection.ty, context->projection.tz);
	nemomatrix_perspective(&projection,
			context->perspective.left,
			context->perspective.right,
			context->perspective.bottom,
			context->perspective.top,
			context->perspective.near,
			context->perspective.far);

	glBindFramebuffer(GL_FRAMEBUFFER, context->fbo);

	glViewport(0, 0,
			nemoshow_canvas_get_viewport_width(canvas),
			nemoshow_canvas_get_viewport_height(canvas));

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

	nemophys_render_3d_softbody(context, &projection, nemoshow_canvas_get_effective_texture(context->video));

	nemolist_for_each(one, &context->obj_list, link) {
		nemophys_render_3d_one(context, &projection, one);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	nemoshow_one_dirty(canvas, NEMOSHOW_REDRAW_DIRTY);
}

static void nemophys_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);
	float x = nemoshow_event_get_x(event) * nemoshow_canvas_get_viewport_sx(canvas);
	float y = nemoshow_event_get_y(event) * nemoshow_canvas_get_viewport_sy(canvas);

	nemoshow_event_update_taps(show, canvas, event);

	if (nemoshow_event_is_pointer_left_down(show, event)) {
	}

	if (nemoshow_event_is_touch_down(show, event)) {
		float x = nemoshow_event_get_x(event) / context->width * 2.0f - 1.0f;
		float y = nemoshow_event_get_y(event) / context->height * 2.0f - 1.0f;
		btCollisionShape *shape;
		struct objone *one;

		if (context->ishapes == 0) {
			shape = new btBoxShape(btVector3(1.0f, 1.0f, 1.0f));

			one = nemophys_one_create(context);
			nemophys_one_set_color(one, 0.0f, 1.0f, 1.0f, 1.0f);
			nemophys_one_set_scale(one, 0.25f, 0.25f, 0.25f);
			nemophys_one_load_cube(one);
		} else {
			shape = new btSphereShape(1.0f);

			one = nemophys_one_create(context);
			nemophys_one_set_color(one, 0.0f, 1.0f, 1.0f, 1.0f);
			nemophys_one_set_scale(one, 0.25f, 0.25f, 0.25f);
			nemophys_one_load_sphere(one, 12, 12, 1.0f);
		}

		shape->setLocalScaling(btVector3(0.25f, 0.25f, 0.25f));
		shape->setMargin(0.001f);

		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3(x, y, 0.0f));
		transform.setRotation(btQuaternion(btVector3(1, 1, 1), random_get_double(0.0f, M_PI)));

		btScalar mass(1.0f);
		btVector3 linertia(0, 0, 0);
		shape->calculateLocalInertia(mass, linertia);

		btDefaultMotionState *motionstate = new btDefaultMotionState(transform);
		btRigidBody::btRigidBodyConstructionInfo bodyinfo(mass, motionstate, shape, linertia);
		btRigidBody *body = new btRigidBody(bodyinfo);
		body->applyCentralForce(btVector3(0, 0, random_get_int(-400, -200)));
		body->setCcdMotionThreshold(1.0f);
		body->setCcdSweptSphereRadius(0.01f);

		context->dynamicsworld->addRigidBody(body);

		nemophys_one_set_body(one, body);
		nemophys_one_set_canvas(one, context->video);

		context->ishapes = (context->ishapes + 1) % 2;
	}

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
	int f;

	context->dynamicsworld->stepSimulation(1.0f / 60.0f, 0);

	for (f = 0; f < context->softbody->m_faces.size(); f++) {
		const btSoftBody::Face &face = context->softbody->m_faces[f];
		const btVector3 x[] = { face.m_n[0]->m_x, face.m_n[1]->m_x, face.m_n[2]->m_x };

		context->vertices[f * 9 + 0] = x[0].getX();
		context->vertices[f * 9 + 1] = x[0].getY();
		context->vertices[f * 9 + 2] = x[0].getZ();
		context->vertices[f * 9 + 3] = x[1].getX();
		context->vertices[f * 9 + 4] = x[1].getY();
		context->vertices[f * 9 + 5] = x[1].getZ();
		context->vertices[f * 9 + 6] = x[2].getX();
		context->vertices[f * 9 + 7] = x[2].getY();
		context->vertices[f * 9 + 8] = x[2].getZ();
	}
}

static void nemophys_leave_show_frame(struct nemoshow *show, uint32_t msecs)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);
}

static void nemophys_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);

	nemoshow_view_resize(context->show, width, height);

	glDeleteFramebuffers(1, &context->fbo);
	glDeleteRenderbuffers(1, &context->dbo);

	gl_create_fbo(
			nemoshow_canvas_get_texture(context->canvas),
			width, height,
			&context->fbo, &context->dbo);

	nemoshow_one_dirty(context->canvas, NEMOSHOW_REDRAW_DIRTY);

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

static void nemophys_dispatch_video_update(struct nemoplay *play, void *data)
{
	struct physcontext *context = (struct physcontext *)data;

	nemoshow_canvas_damage_all(context->video);
	nemoshow_dispatch_frame(context->show);
}

static void nemophys_dispatch_video_done(struct nemoplay *play, void *data)
{
	struct physcontext *context = (struct physcontext *)data;

	nemoplay_back_destroy_decoder(context->decoderback);
	nemoplay_back_destroy_audio(context->audioback);
	nemoplay_back_destroy_video(context->videoback);
	nemoplay_destroy(context->play);

	context->imovies = (context->imovies + 1) % nemofs_dir_get_filecount(context->movies);

	context->play = nemoplay_create();
	nemoplay_load_media(context->play, nemofs_dir_get_filepath(context->movies, context->imovies));

	nemoshow_canvas_set_size(context->video,
			nemoplay_get_video_width(context->play),
			nemoplay_get_video_height(context->play));

	context->decoderback = nemoplay_back_create_decoder(context->play);
	context->audioback = nemoplay_back_create_audio_by_ao(context->play);
	context->videoback = nemoplay_back_create_video_by_timer(context->play, context->tool);
	nemoplay_back_set_video_texture(context->videoback,
			nemoshow_canvas_get_texture(context->video),
			nemoplay_get_video_width(context->play),
			nemoplay_get_video_height(context->play));
	nemoplay_back_set_video_update(context->videoback, nemophys_dispatch_video_update);
	nemoplay_back_set_video_done(context->videoback, nemophys_dispatch_video_done);
	nemoplay_back_set_video_data(context->videoback, context);
}

static int nemophys_prepare_bullet(struct physcontext *context)
{
	context->worldinfo = new btSoftBodyWorldInfo();
	context->configuration = new btSoftBodyRigidBodyCollisionConfiguration();
	context->dispatcher = new btCollisionDispatcher(context->configuration);
	context->worldinfo->m_dispatcher = context->dispatcher;

	btVector3 min(-1000.0f, -1000.0f, -1000.0f);
	btVector3 max(1000.0f, 1000.0f, 1000.0f);

	context->broadphase = new btAxisSweep3(min, max, 32766);
	context->worldinfo->m_broadphase = context->broadphase;

	context->solver = new btSequentialImpulseConstraintSolver();

	context->dynamicsworld = new btSoftRigidDynamicsWorld(context->dispatcher, context->broadphase, context->solver, context->configuration, NULL);
	context->dynamicsworld->setGravity(btVector3(0.0f, 9.8f, 0.0f));
	context->worldinfo->m_gravity.setValue(0.0f, 9.8f, 0.0f);
	context->worldinfo->m_sparsesdf.Initialize();

	return 0;
}

static void nemophys_finish_bullet(struct physcontext *context)
{
	delete context->solver;
	delete context->broadphase;
	delete context->dispatcher;
	delete context->configuration;
	delete context->worldinfo;
	delete context->dynamicsworld;
}

static int nemophys_prepare_softbody(struct physcontext *context, int columns, int rows)
{
	btVector3 corners[4] = {
		{ -1.0f, 1.0f, -1.25f },
		{ 1.0f, 1.0f, -1.25f },
		{ -1.0f, -1.0f, -1.25f },
		{ 1.0f, -1.0f, -1.25f },
	};

	context->vertices = (float *)malloc(sizeof(float[3]) * columns * rows * 12);
	context->texcoords = (float *)malloc(sizeof(float[2]) * columns * rows * 12);

	context->softbody = btSoftBodyHelpers::CreatePatchUV(
			*context->worldinfo,
			corners[0],
			corners[1],
			corners[2],
			corners[3],
			columns, rows,
			4 + 8, true,
			context->texcoords);
	context->softbody->getCollisionShape()->setMargin(0.001f);

	context->nvertices = context->softbody->m_faces.size() * 3;

	btSoftBody::Material *material = context->softbody->appendMaterial();
	material->m_kLST = 0.001f;

	context->softbody->m_cfg.piterations = 10;
	context->softbody->m_cfg.citerations = 10;
	context->softbody->m_cfg.diterations = 10;
	context->softbody->m_cfg.collisions = btSoftBody::fCollision::CL_SS + btSoftBody::fCollision::SDF_RS;
	context->softbody->generateBendingConstraints(2, material);
	context->softbody->generateClusters(64);
	context->softbody->setTotalMass(1.0f);

	context->dynamicsworld->addSoftBody(context->softbody);

	return 0;
}

static void nemophys_finish_softbody(struct physcontext *context)
{
	free(context->vertices);
	free(context->texcoords);

	delete context->softbody;
}

static int nemophys_prepare_floor(struct physcontext *context)
{
	btCollisionShape *shape;

	shape = new btBoxShape(btVector3(100.0f, 5.0f, 100.0f));
	shape->setMargin(0.001f);

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(0.0f, 6.0f, 0.0f));

	btScalar mass(0.0f);
	btVector3 linertia(0, 0, 0);
	btDefaultMotionState *motionstate = new btDefaultMotionState(transform);
	btRigidBody::btRigidBodyConstructionInfo bodyinfo(mass, motionstate, shape, linertia);
	btRigidBody *body = new btRigidBody(bodyinfo);

	context->dynamicsworld->addRigidBody(body);

	return 0;
}

static void nemophys_finish_floor(struct physcontext *context)
{
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
		"  gl_FragColor = texture2D(texture, vtexcoord);\n"
		"}\n";
	static const char *vertexshader_solid =
		"uniform mat4 projection;\n"
		"uniform mat4 vtransform;\n"
		"attribute vec3 position;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = projection * vtransform * vec4(position.xyz, 1.0);\n"
		"}\n";
	static const char *fragmentshader_solid =
		"precision mediump float;\n"
		"uniform vec4 color;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = color;\n"
		"}\n";

	gl_create_fbo(
			nemoshow_canvas_get_texture(context->canvas),
			width, height,
			&context->fbo, &context->dbo);

	context->programs[0] = gl_compile_program(vertexshader_texture, fragmentshader_texture, NULL, NULL);
	context->programs[1] = gl_compile_program(vertexshader_solid, fragmentshader_solid, NULL, NULL);

	context->uprojection0 = glGetUniformLocation(context->programs[0], "projection");
	context->uvtransform0 = glGetUniformLocation(context->programs[0], "vtransform");
	context->utexture0 = glGetUniformLocation(context->programs[0], "texture");
	context->uprojection1 = glGetUniformLocation(context->programs[1], "projection");
	context->uvtransform1 = glGetUniformLocation(context->programs[1], "vtransform");
	context->ucolor1 = glGetUniformLocation(context->programs[1], "color");

	return 0;
}

static void nemophys_finish_opengl(struct physcontext *context)
{
	glDeleteFramebuffers(1, &context->fbo);
	glDeleteRenderbuffers(1, &context->dbo);

	glDeleteProgram(context->programs[0]);
	glDeleteProgram(context->programs[1]);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "fullscreen",			required_argument,			NULL,		'f' },
		{ "video",					required_argument,			NULL,		'v' },
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
	char *videopath = NULL;
	int width = 800;
	int height = 800;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "f:v:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				fullscreen = strdup(optarg);
				break;

			case 'v':
				videopath = strdup(optarg);
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

	context->perspective.left = -1.0f;
	context->perspective.right = 1.0f;
	context->perspective.bottom = -1.0f;
	context->perspective.top = 1.0f;
	context->perspective.near = 1.0f;
	context->perspective.far = 4.0f;

	nemolist_init(&context->obj_list);

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

	context->video = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_attach_one(show, canvas);

	nemophys_prepare_opengl(context, width, height);
	nemophys_prepare_bullet(context);
	nemophys_prepare_softbody(context, 24, 24);
	nemophys_prepare_floor(context);

	if (videopath != NULL) {
		if (os_check_path_is_directory(videopath) != 0) {
			context->movies = nemofs_dir_create(videopath, 32);
			nemofs_dir_scan_extension(context->movies, "mp4");
			nemofs_dir_scan_extension(context->movies, "avi");
			nemofs_dir_scan_extension(context->movies, "ts");
		} else {
			context->movies = nemofs_dir_create(NULL, 32);
			nemofs_dir_insert_file(context->movies, videopath);
		}

		context->play = nemoplay_create();
		nemoplay_load_media(context->play, nemofs_dir_get_filepath(context->movies, context->imovies));

		nemoshow_canvas_set_size(context->video,
				nemoplay_get_video_width(context->play),
				nemoplay_get_video_height(context->play));

		context->decoderback = nemoplay_back_create_decoder(context->play);
		context->audioback = nemoplay_back_create_audio_by_ao(context->play);
		context->videoback = nemoplay_back_create_video_by_timer(context->play, tool);
		nemoplay_back_set_video_texture(context->videoback,
				nemoshow_canvas_get_texture(context->video),
				nemoplay_get_video_width(context->play),
				nemoplay_get_video_height(context->play));
		nemoplay_back_set_video_update(context->videoback, nemophys_dispatch_video_update);
		nemoplay_back_set_video_done(context->videoback, nemophys_dispatch_video_done);
		nemoplay_back_set_video_data(context->videoback, context);
	}

	trans = nemoshow_transition_create(NEMOSHOW_LINEAR_EASE, 18000, 0);
	nemoshow_transition_dirty_one(trans, context->canvas, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_attach_transition(show, trans);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemophys_finish_floor(context);
	nemophys_finish_softbody(context);
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
