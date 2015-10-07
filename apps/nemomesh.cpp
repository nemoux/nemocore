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
#include <oshelper.h>
#include <nemomatrix.h>
#include <nemometro.h>
#include <nemobox.h>
#include <nemomisc.h>

#define	NEMOMESH_SHADERS_MAX			(2)

struct meshone {
	struct nemomatrix modelview;

	GLuint varray;
	GLuint vbuffer;
	GLuint vindex;

	GLuint garray;
	GLuint gbuffer;

	float *lines;
	int nlines, slines;

	float *meshes;
	int nmeshes, smeshes;

	float *guides;
	int nguides, sguides;

	int on_guides;

	GLenum mode;
	int elements;

	float boundingbox[6];

	float sx, sy, sz;
	float tx, ty, tz;

	struct nemovector avec, cvec;
	struct nemoquaternion squat, cquat;
};

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

	struct meshone *one;
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

static struct meshone *nemomesh_create_one(const char *filepath, const char *basepath)
{
	struct meshone *one;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string r;
	uint32_t base = 0;
	float minx = FLT_MAX, miny = FLT_MAX, minz = FLT_MAX, maxx = FLT_MIN, maxy = FLT_MIN, maxz = FLT_MIN;
	float max;
	int i, j;

	one = (struct meshone *)malloc(sizeof(struct meshone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct meshone));

	nemoquaternion_init_identity(&one->cquat);

	one->lines = (float *)malloc(sizeof(float) * 16);
	one->nlines = 0;
	one->slines = 16;

	one->meshes = (float *)malloc(sizeof(float) * 16);
	one->nmeshes = 0;
	one->smeshes = 16;

	one->guides = (float *)malloc(sizeof(float) * 16);
	one->nguides = 0;
	one->sguides = 16;

	r = tinyobj::LoadObj(shapes, materials, filepath, basepath);
	if (!r.empty())
		exit(1);

#if	0
	for (i = 0; i < shapes.size(); i++) {
		for (j = 0; j < shapes[i].mesh.indices.size() / 3; j++) {
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, shapes[i].mesh.indices[j * 3 + 0] + base);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, shapes[i].mesh.indices[j * 3 + 1] + base);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, shapes[i].mesh.indices[j * 3 + 0] + base);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, shapes[i].mesh.indices[j * 3 + 2] + base);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, shapes[i].mesh.indices[j * 3 + 1] + base);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, shapes[i].mesh.indices[j * 3 + 2] + base);

			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, shapes[i].mesh.indices[j * 3 + 0] + base);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, shapes[i].mesh.indices[j * 3 + 1] + base);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, shapes[i].mesh.indices[j * 3 + 2] + base);
		}

		if (shapes[i].mesh.normals.size() != shapes[i].mesh.positions.size()) {
			for (j = 0; j < shapes[i].mesh.positions.size() / 3; j++, base++) {
				struct nemovector v0 = {
					shapes[i].mesh.positions[j * 3 + 0],
					shapes[i].mesh.positions[j * 3 + 1],
					shapes[i].mesh.positions[j * 3 + 2],
					1.0f
				};

				nemovector_normalize(&v0);

				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, shapes[i].mesh.positions[j * 3 + 0]);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, shapes[i].mesh.positions[j * 3 + 1]);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, shapes[i].mesh.positions[j * 3 + 2]);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, v0.f[0]);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, v0.f[1]);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, v0.f[2]);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, 0.0f);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, 1.0f);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, 1.0f);

				if (shapes[i].mesh.positions[j * 3 + 0] < minx)
					minx = shapes[i].mesh.positions[j * 3 + 0];
				if (shapes[i].mesh.positions[j * 3 + 1] < miny)
					miny = shapes[i].mesh.positions[j * 3 + 1];
				if (shapes[i].mesh.positions[j * 3 + 2] < minz)
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
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, shapes[i].mesh.positions[j * 3 + 0]);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, shapes[i].mesh.positions[j * 3 + 1]);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, shapes[i].mesh.positions[j * 3 + 2]);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, shapes[i].mesh.normals[j * 3 + 0]);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, shapes[i].mesh.normals[j * 3 + 1]);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, shapes[i].mesh.normals[j * 3 + 2]);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, 0.0f);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, 1.0f);
				NEMOBOX_APPEND(one->vertices, one->svertices, one->nvertices, 1.0f);

				if (shapes[i].mesh.positions[j * 3 + 0] < minx)
					minx = shapes[i].mesh.positions[j * 3 + 0];
				if (shapes[i].mesh.positions[j * 3 + 1] < miny)
					miny = shapes[i].mesh.positions[j * 3 + 1];
				if (shapes[i].mesh.positions[j * 3 + 2] < minz)
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
#else
	for (i = 0; i < shapes.size(); i++) {
		for (j = 0; j < shapes[i].mesh.indices.size() / 3; j++) {
			uint32_t v0 = shapes[i].mesh.indices[j * 3 + 0];
			uint32_t v1 = shapes[i].mesh.indices[j * 3 + 1];
			uint32_t v2 = shapes[i].mesh.indices[j * 3 + 2];
			float p0[3], p1[3], p2[3];
			float n0[3], n1[3], n2[3];
			float r = 1.0f, g = 1.0f, b = 1.0f;

			p0[0] = shapes[i].mesh.positions[v0 * 3 + 0];
			p0[1] = shapes[i].mesh.positions[v0 * 3 + 1];
			p0[2] = shapes[i].mesh.positions[v0 * 3 + 2];

			p1[0] = shapes[i].mesh.positions[v1 * 3 + 0];
			p1[1] = shapes[i].mesh.positions[v1 * 3 + 1];
			p1[2] = shapes[i].mesh.positions[v1 * 3 + 2];

			p2[0] = shapes[i].mesh.positions[v2 * 3 + 0];
			p2[1] = shapes[i].mesh.positions[v2 * 3 + 1];
			p2[2] = shapes[i].mesh.positions[v2 * 3 + 2];

			if (shapes[i].mesh.normals.size() > 0) {
				n0[0] = shapes[i].mesh.normals[v0 * 3 + 0];
				n0[1] = shapes[i].mesh.normals[v0 * 3 + 1];
				n0[2] = shapes[i].mesh.normals[v0 * 3 + 2];

				n1[0] = shapes[i].mesh.normals[v1 * 3 + 0];
				n1[1] = shapes[i].mesh.normals[v1 * 3 + 1];
				n1[2] = shapes[i].mesh.normals[v1 * 3 + 2];

				n2[0] = shapes[i].mesh.normals[v2 * 3 + 0];
				n2[1] = shapes[i].mesh.normals[v2 * 3 + 1];
				n2[2] = shapes[i].mesh.normals[v2 * 3 + 2];
			} else {
				struct nemovector t0 = { p0[0], p0[1], p0[2] };
				struct nemovector t1 = { p1[0], p1[1], p1[2] };
				struct nemovector t2 = { p2[0], p2[1], p2[2] };
				struct nemovector s0, s1;

				s0.f[0] = t1.f[0] - t0.f[0];
				s0.f[1] = t1.f[1] - t0.f[1];
				s0.f[2] = t1.f[2] - t0.f[2];

				s1.f[0] = t2.f[0] - t0.f[0];
				s1.f[1] = t2.f[1] - t0.f[1];
				s1.f[2] = t2.f[2] - t0.f[2];

				nemovector_cross(&s0, &s1);
				nemovector_normalize(&s0);

				n0[0] = n1[0] = n2[0] = s0.f[0];
				n0[1] = n1[1] = n2[1] = s0.f[1];
				n0[2] = n1[2] = n2[2] = s0.f[2];
			}

			if (shapes[i].mesh.material_ids.size() > 0) {
				int32_t m = shapes[i].mesh.material_ids[j / 3];

				if (m >= 0) {
					r = materials[m].diffuse[0];
					g = materials[m].diffuse[1];
					b = materials[m].diffuse[2];
				}
			}

			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, p0[0]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, p0[1]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, p0[2]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, n0[0]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, n0[1]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, n0[2]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, r);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, g);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, b);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, p1[0]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, p1[1]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, p1[2]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, n1[0]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, n1[1]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, n1[2]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, r);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, g);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, b);

			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, p0[0]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, p0[1]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, p0[2]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, n0[0]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, n0[1]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, n0[2]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, r);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, g);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, b);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, p2[0]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, p2[1]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, p2[2]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, n2[0]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, n2[1]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, n2[2]);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, r);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, g);
			NEMOBOX_APPEND(one->lines, one->slines, one->nlines, b);

			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, p0[0]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, p0[1]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, p0[2]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, n0[0]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, n0[1]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, n0[2]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, r);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, g);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, b);

			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, p1[0]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, p1[1]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, p1[2]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, n1[0]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, n1[1]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, n1[2]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, r);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, g);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, b);

			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, p2[0]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, p2[1]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, p2[2]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, n2[0]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, n2[1]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, n2[2]);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, r);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, g);
			NEMOBOX_APPEND(one->meshes, one->smeshes, one->nmeshes, b);

			minx = MIN(p0[0], minx);
			minx = MIN(p1[0], minx);
			minx = MIN(p2[0], minx);

			miny = MIN(p0[1], miny);
			miny = MIN(p1[1], miny);
			miny = MIN(p2[1], miny);

			minz = MIN(p0[2], minz);
			minz = MIN(p1[2], minz);
			minz = MIN(p2[2], minz);

			maxx = MAX(p0[0], maxx);
			maxx = MAX(p1[0], maxx);
			maxx = MAX(p2[0], maxx);

			maxy = MAX(p0[1], maxy);
			maxy = MAX(p1[1], maxy);
			maxy = MAX(p2[1], maxy);

			maxz = MAX(p0[2], maxz);
			maxz = MAX(p1[2], maxz);
			maxz = MAX(p2[2], maxz);
		}
	}
#endif

#define	NEMOMESH_APPEND_GUIDE_VERTEX(x0, x1, y0, y1, z0, z1, xn, yn, zn, r, g, b)	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, x0);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, y0);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, z0);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, xn);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, yn);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, zn);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, r);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, g);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, b);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, x1);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, y1);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, z1);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, xn);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, yn);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, zn);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, r);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, g);	\
	NEMOBOX_APPEND(one->guides, one->sguides, one->nguides, b)

	NEMOMESH_APPEND_GUIDE_VERTEX(minx, maxx, miny, miny, minz, minz, 0.0f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f);
	NEMOMESH_APPEND_GUIDE_VERTEX(minx, maxx, maxy, maxy, minz, minz, 0.0f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f);
	NEMOMESH_APPEND_GUIDE_VERTEX(minx, maxx, miny, miny, maxz, maxz, 0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f);
	NEMOMESH_APPEND_GUIDE_VERTEX(minx, maxx, maxy, maxy, maxz, maxz, 0.0f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f);

	NEMOMESH_APPEND_GUIDE_VERTEX(minx, minx, miny, maxy, minz, minz, -0.5f, 0.0f, -0.5f, 0.0f, 1.0f, 0.0f);
	NEMOMESH_APPEND_GUIDE_VERTEX(maxx, maxx, miny, maxy, minz, minz, 0.5f, 0.0f, -0.5f, 0.0f, 1.0f, 0.0f);
	NEMOMESH_APPEND_GUIDE_VERTEX(minx, minx, miny, maxy, maxz, maxz, -0.5f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f);
	NEMOMESH_APPEND_GUIDE_VERTEX(maxx, maxx, miny, maxy, maxz, maxz, 0.5f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f);

	NEMOMESH_APPEND_GUIDE_VERTEX(minx, minx, miny, miny, minz, maxz, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f);
	NEMOMESH_APPEND_GUIDE_VERTEX(maxx, maxx, miny, miny, minz, maxz, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f);
	NEMOMESH_APPEND_GUIDE_VERTEX(minx, minx, maxy, maxy, minz, maxz, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f);
	NEMOMESH_APPEND_GUIDE_VERTEX(maxx, maxx, maxy, maxy, minz, maxz, 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f);

	max = MAX(maxx - minx, MAX(maxy - miny, maxz - minz));

	one->sx = 2.0f / max;
	one->sy = 2.0f / max;
	one->sz = 2.0f / max;

	one->tx = -(maxx + minx) / 2.0f;
	one->ty = -(maxy + miny) / 2.0f;
	one->tz = -(maxz + minz) / 2.0f;

	one->boundingbox[0] = minx;
	one->boundingbox[1] = maxx;
	one->boundingbox[2] = miny;
	one->boundingbox[3] = maxy;
	one->boundingbox[4] = minz;
	one->boundingbox[5] = maxz;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glGenVertexArrays(1, &one->varray);
	glBindVertexArray(one->varray);

	glGenBuffers(1, &one->vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, one->vbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void *)sizeof(GLfloat[3]));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void *)sizeof(GLfloat[6]));
	glEnableVertexAttribArray(2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * one->nmeshes, one->meshes, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &one->vindex);

	glBindVertexArray(0);

	glGenVertexArrays(1, &one->garray);
	glBindVertexArray(one->garray);

	glGenBuffers(1, &one->gbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, one->gbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void *)sizeof(GLfloat[3]));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void *)sizeof(GLfloat[6]));
	glEnableVertexAttribArray(2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * one->nguides, one->guides, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	return one;
}

static void nemomesh_destroy_one(struct meshone *one)
{
	glDeleteBuffers(1, &one->vbuffer);
	glDeleteBuffers(1, &one->vindex);
	glDeleteVertexArrays(1, &one->varray);

	glDeleteBuffers(1, &one->gbuffer);
	glDeleteVertexArrays(1, &one->garray);

	free(one->lines);
	free(one->meshes);
	free(one->guides);
}

static void nemomesh_prepare_buffer(struct meshone *one, GLenum mode, float *buffers, int elements)
{
	glBindVertexArray(one->varray);

	glBindBuffer(GL_ARRAY_BUFFER, one->vbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * elements, buffers, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	one->mode = mode;
	one->elements = elements;
}

static void nemomesh_prepare_index(struct meshone *one, GLenum mode, uint32_t *buffers, int elements)
{
	glBindVertexArray(one->varray);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, one->vindex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * elements, buffers, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	one->mode = mode;
	one->elements = elements;
}

static void nemomesh_update_one(struct meshone *one)
{
	nemomatrix_init_identity(&one->modelview);
	nemomatrix_translate_xyz(&one->modelview, one->tx, one->ty, one->tz);
	nemomatrix_multiply_quaternion(&one->modelview, &one->cquat);
	nemomatrix_scale_xyz(&one->modelview, one->sx, one->sy, one->sz);
}

static void nemomesh_render(struct meshcontext *context)
{
	struct meshone *one;
	struct nemomatrix matrix;
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

	one = context->one;

	matrix = one->modelview;
	nemomatrix_multiply(&matrix, &context->projection);

	glUniformMatrix4fv(context->umvp, 1, GL_FALSE, (GLfloat *)matrix.d);
	glUniformMatrix4fv(context->umodelview, 1, GL_FALSE, (GLfloat *)one->modelview.d);

	glUniform4fv(context->ucolor, 1, color);

#if	0
	glBindVertexArray(one->varray);
	glDrawElements(one->mode, one->elements, GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);
#else
	glBindVertexArray(one->varray);
	glDrawArrays(one->mode, 0, one->elements);
	glBindVertexArray(0);
#endif

	if (one->on_guides != 0) {
		glPointSize(10.0f);

		glBindVertexArray(one->garray);
		glDrawArrays(GL_LINES, 0, one->nguides);
		glBindVertexArray(0);
	}

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

static void nemomesh_compute_quaternion(struct meshone *one)
{
	struct nemovector v = one->avec;
	float dot = nemovector_dot(&one->avec, &one->cvec);
	float angle = acosf(dot);

	if (isnan(angle) == 0) {
		nemovector_cross(&v, &one->cvec);

		nemoquaternion_make_with_angle_axis(&one->cquat, angle * 2.0f, v.f[0], v.f[1], v.f[2]);
		nemoquaternion_normalize(&one->cquat);
		nemoquaternion_multiply(&one->cquat, &one->squat);
	}
}

static void nemomesh_unproject(struct meshcontext *context, struct meshone *one, float x, float y, float z, float *out)
{
	struct nemomatrix matrix;
	struct nemomatrix inverse;
	struct nemovector in;

	matrix = one->modelview;
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

static int nemomesh_pick_one(struct meshcontext *context, struct meshone *one, float x, float y)
{
	float near[3], far[3];
	float rayorg[3];
	float rayvec[3];
	float raylen;
	float mint, maxt;

	nemomesh_unproject(context, one, x, y, -1.0f, near);
	nemomesh_unproject(context, one, x, y, 1.0f, far);

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

	return nemometro_cube_intersect(one->boundingbox, rayorg, rayvec, &mint, &maxt);
}

static void nemomesh_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	struct meshcontext *context = (struct meshcontext *)nemotale_get_userdata(tale);
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (nemotale_is_touch_down(tale, event, type)) {
			struct meshone *one = context->one;
			int plane = nemomesh_pick_one(context, one, event->x, event->y);

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

					one->on_guides = 1;

					nemocanvas_dispatch_frame(context->canvas);
				} else if (nemotale_is_triple_taps(tale, event, type)) {
					struct meshone *one = context->one;

					one->avec.f[0] = event->taps[2]->x;
					one->avec.f[1] = event->taps[2]->y;
					one->avec.f[2] = 0.0f;

					nemomesh_project_onto_surface(context->width, context->height, &one->avec);

					one->squat = one->cquat;
				}
			}
		} else if (nemotale_is_touch_motion(tale, event, type)) {
			nemotale_event_update_node_taps(tale, node, event, type);

			if (nemotale_is_triple_taps(tale, event, type)) {
				struct meshone *one = context->one;

				one->cvec.f[0] = event->taps[2]->x;
				one->cvec.f[1] = event->taps[2]->y;
				one->cvec.f[2] = 0.0f;

				nemomesh_project_onto_surface(context->width, context->height, &one->cvec);
				nemomesh_compute_quaternion(one);

				nemocanvas_dispatch_frame(context->canvas);
			}
		} else if (nemotale_is_touch_up(tale, event, type)) {
			nemotale_event_update_node_taps(tale, node, event, type);

			if (nemotale_is_double_taps(tale, event, type)) {
				struct meshone *one = context->one;

				one->on_guides = 0;

				nemocanvas_dispatch_frame(context->canvas);
			}
		}

		if (nemotale_is_single_click(tale, event, type)) {
			struct meshone *one = context->one;
			int plane = nemomesh_pick_one(context, one, event->x, event->y);

			if (plane != NEMO_METRO_NONE_PLANE) {
				if (one->mode == GL_TRIANGLES)
					nemomesh_prepare_buffer(one, GL_LINES, one->lines, one->nlines);
				else
					nemomesh_prepare_buffer(one, GL_TRIANGLES, one->meshes, one->nmeshes);

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

	nemomesh_update_one(context->one);

	nemomesh_render(context);

	nemotale_node_damage_all(context->node);

	nemotale_composite_egl(context->tale, NULL);
}

static void nemomesh_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height, int32_t fixed)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct meshcontext *context = (struct meshcontext *)nemotale_get_userdata(tale);

	if (width == 0 || height == 0)
		return;

	if (width < nemotale_get_minimum_width(tale) * 2 || height < nemotale_get_minimum_height(tale) * 2) {
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
	struct meshone *one;
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
	nemocanvas_set_userdata(NTEGL_CANVAS(canvas), context);
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

	context->one = one = nemomesh_create_one(filepath, basepath);
	nemomesh_prepare_buffer(one, GL_LINES, one->lines, one->nlines);

	nemocanvas_dispatch_frame(NTEGL_CANVAS(canvas));

	nemotool_run(tool);

	nemomesh_destroy_one(context->one);

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
