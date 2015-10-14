#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <float.h>
#include <math.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <tiny_obj_loader.h>

#include <meshhelper.h>
#include <nemobox.h>

struct nemomesh *nemomesh_create_object(const char *filepath, const char *basepath)
{
	struct nemomesh *mesh;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string r;
	uint32_t base = 0;
	float minx = FLT_MAX, miny = FLT_MAX, minz = FLT_MAX, maxx = FLT_MIN, maxy = FLT_MIN, maxz = FLT_MIN;
	float max;
	int i, j;

	r = tinyobj::LoadObj(shapes, materials, filepath, basepath);
	if (!r.empty())
		return NULL;

	mesh = (struct nemomesh *)malloc(sizeof(struct nemomesh));
	if (mesh == NULL)
		return NULL;
	memset(mesh, 0, sizeof(struct nemomesh));

	nemoquaternion_init_identity(&mesh->cquat);

	mesh->lines = (float *)malloc(sizeof(float) * 16);
	mesh->nlines = 0;
	mesh->slines = 16;

	mesh->meshes = (float *)malloc(sizeof(float) * 16);
	mesh->nmeshes = 0;
	mesh->smeshes = 16;

	mesh->guides = (float *)malloc(sizeof(float) * 16);
	mesh->nguides = 0;
	mesh->sguides = 16;

#if	0
	for (i = 0; i < shapes.size(); i++) {
		for (j = 0; j < shapes[i].mesh.indices.size() / 3; j++) {
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, shapes[i].mesh.indices[j * 3 + 0] + base);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, shapes[i].mesh.indices[j * 3 + 1] + base);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, shapes[i].mesh.indices[j * 3 + 0] + base);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, shapes[i].mesh.indices[j * 3 + 2] + base);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, shapes[i].mesh.indices[j * 3 + 1] + base);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, shapes[i].mesh.indices[j * 3 + 2] + base);

			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, shapes[i].mesh.indices[j * 3 + 0] + base);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, shapes[i].mesh.indices[j * 3 + 1] + base);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, shapes[i].mesh.indices[j * 3 + 2] + base);
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

				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, shapes[i].mesh.positions[j * 3 + 0]);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, shapes[i].mesh.positions[j * 3 + 1]);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, shapes[i].mesh.positions[j * 3 + 2]);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, v0.f[0]);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, v0.f[1]);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, v0.f[2]);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, 0.0f);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, 1.0f);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, 1.0f);

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
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, shapes[i].mesh.positions[j * 3 + 0]);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, shapes[i].mesh.positions[j * 3 + 1]);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, shapes[i].mesh.positions[j * 3 + 2]);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, shapes[i].mesh.normals[j * 3 + 0]);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, shapes[i].mesh.normals[j * 3 + 1]);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, shapes[i].mesh.normals[j * 3 + 2]);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, 0.0f);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, 1.0f);
				NEMOBOX_APPEND(mesh->vertices, mesh->svertices, mesh->nvertices, 1.0f);

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

			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, p0[0]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, p0[1]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, p0[2]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, n0[0]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, n0[1]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, n0[2]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, r);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, g);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, b);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, p1[0]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, p1[1]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, p1[2]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, n1[0]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, n1[1]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, n1[2]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, r);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, g);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, b);

			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, p0[0]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, p0[1]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, p0[2]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, n0[0]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, n0[1]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, n0[2]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, r);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, g);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, b);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, p2[0]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, p2[1]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, p2[2]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, n2[0]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, n2[1]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, n2[2]);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, r);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, g);
			NEMOBOX_APPEND(mesh->lines, mesh->slines, mesh->nlines, b);

			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, p0[0]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, p0[1]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, p0[2]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, n0[0]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, n0[1]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, n0[2]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, r);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, g);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, b);

			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, p1[0]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, p1[1]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, p1[2]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, n1[0]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, n1[1]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, n1[2]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, r);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, g);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, b);

			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, p2[0]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, p2[1]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, p2[2]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, n2[0]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, n2[1]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, n2[2]);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, r);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, g);
			NEMOBOX_APPEND(mesh->meshes, mesh->smeshes, mesh->nmeshes, b);

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
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, x0);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, y0);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, z0);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, xn);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, yn);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, zn);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, r);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, g);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, b);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, x1);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, y1);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, z1);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, xn);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, yn);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, zn);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, r);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, g);	\
	NEMOBOX_APPEND(mesh->guides, mesh->sguides, mesh->nguides, b)

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

	mesh->sx = 2.0f / max;
	mesh->sy = 2.0f / max;
	mesh->sz = 2.0f / max;

	mesh->tx = -(maxx + minx) / 2.0f;
	mesh->ty = -(maxy + miny) / 2.0f;
	mesh->tz = -(maxz + minz) / 2.0f;

	mesh->boundingbox[0] = minx;
	mesh->boundingbox[1] = maxx;
	mesh->boundingbox[2] = miny;
	mesh->boundingbox[3] = maxy;
	mesh->boundingbox[4] = minz;
	mesh->boundingbox[5] = maxz;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glGenVertexArrays(1, &mesh->varray);
	glBindVertexArray(mesh->varray);

	glGenBuffers(1, &mesh->vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void *)sizeof(GLfloat[3]));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void *)sizeof(GLfloat[6]));
	glEnableVertexAttribArray(2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh->nmeshes, mesh->meshes, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &mesh->vindex);

	glBindVertexArray(0);

	glGenVertexArrays(1, &mesh->garray);
	glBindVertexArray(mesh->garray);

	glGenBuffers(1, &mesh->gbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->gbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void *)sizeof(GLfloat[3]));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void *)sizeof(GLfloat[6]));
	glEnableVertexAttribArray(2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh->nguides, mesh->guides, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	return mesh;
}

void nemomesh_destroy_object(struct nemomesh *mesh)
{
	glDeleteBuffers(1, &mesh->vbuffer);
	glDeleteBuffers(1, &mesh->vindex);
	glDeleteVertexArrays(1, &mesh->varray);

	glDeleteBuffers(1, &mesh->gbuffer);
	glDeleteVertexArrays(1, &mesh->garray);

	free(mesh->lines);
	free(mesh->meshes);
	free(mesh->guides);
}

void nemomesh_prepare_buffer(struct nemomesh *mesh, GLenum mode, float *buffers, int elements)
{
	glBindVertexArray(mesh->varray);

	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * elements, buffers, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	mesh->mode = mode;
	mesh->elements = elements;
}

void nemomesh_prepare_index(struct nemomesh *mesh, GLenum mode, uint32_t *buffers, int elements)
{
	glBindVertexArray(mesh->varray);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->vindex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * elements, buffers, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	mesh->mode = mode;
	mesh->elements = elements;
}

void nemomesh_update_transform(struct nemomesh *mesh)
{
	nemomatrix_init_identity(&mesh->modelview);
	nemomatrix_translate_xyz(&mesh->modelview, mesh->tx, mesh->ty, mesh->tz);
	nemomatrix_multiply_quaternion(&mesh->modelview, &mesh->cquat);
	nemomatrix_scale_xyz(&mesh->modelview, mesh->sx, mesh->sy, mesh->sz);
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

static void nemomesh_compute_quaternion(struct nemomesh *mesh)
{
	struct nemovector v = mesh->avec;
	float dot = nemovector_dot(&mesh->avec, &mesh->cvec);
	float angle = acosf(dot);

	if (isnan(angle) == 0) {
		nemovector_cross(&v, &mesh->cvec);

		nemoquaternion_make_with_angle_axis(&mesh->cquat, angle * 2.0f, v.f[0], v.f[1], v.f[2]);
		nemoquaternion_normalize(&mesh->cquat);
		nemoquaternion_multiply(&mesh->cquat, &mesh->squat);
	}
}

void nemomesh_reset_quaternion(struct nemomesh *mesh, int32_t width, int32_t height, float x, float y)
{
	mesh->avec.f[0] = x;
	mesh->avec.f[1] = y;
	mesh->avec.f[2] = 0.0f;

	nemomesh_project_onto_surface(width, height, &mesh->avec);

	mesh->squat = mesh->cquat;
}

void nemomesh_update_quaternion(struct nemomesh *mesh, int32_t width, int32_t height, float x, float y)
{
	mesh->cvec.f[0] = x;
	mesh->cvec.f[1] = y;
	mesh->cvec.f[2] = 0.0f;

	nemomesh_project_onto_surface(width, height, &mesh->cvec);
	nemomesh_compute_quaternion(mesh);
}
