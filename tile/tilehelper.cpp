#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <float.h>
#include <math.h>

#include <tiny_obj_loader.h>
#include <nemotile.h>
#include <tilehelper.hpp>
#include <nemomatrix.h>
#include <nemomisc.h>

struct tileone *nemotile_one_create_mesh(const char *filepath, const char *basepath)
{
	struct tileone *one;
	struct nemomatrix transform;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string r;
	float minx = FLT_MAX, miny = FLT_MAX, minz = FLT_MAX, maxx = FLT_MIN, maxy = FLT_MIN, maxz = FLT_MIN;
	float max;
	int i, j, idx;

	r = tinyobj::LoadObj(shapes, materials, filepath, basepath);
	if (!r.empty())
		return NULL;

	one = (struct tileone *)malloc(sizeof(struct tileone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct tileone));

	for (i = 0; i < shapes.size(); i++) {
		one->count += shapes[i].mesh.indices.size();
	}

	one->type = GL_TRIANGLES;

	one->vertices = (float *)malloc(sizeof(float[3]) * one->count);
	one->texcoords = (float *)malloc(sizeof(float[2]) * one->count);
	one->normals = (float *)malloc(sizeof(float[3]) * one->count);

	for (i = 0, idx = 0; i < shapes.size(); i++) {
		for (j = 0; j < shapes[i].mesh.indices.size() / 3; j++, idx++) {
			uint32_t v0 = shapes[i].mesh.indices[j * 3 + 0];
			uint32_t v1 = shapes[i].mesh.indices[j * 3 + 1];
			uint32_t v2 = shapes[i].mesh.indices[j * 3 + 2];
			float p0[3], p1[3], p2[3];
			float n0[3], n1[3], n2[3];
			float s0[3], s1[3];
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
				NEMOVECTOR_SUB(s0, p1, p0);
				NEMOVECTOR_SUB(s1, p2, p0);
				NEMOVECTOR_CROSS(s0, s0, s1);
				NEMOVECTOR_NORMALIZE(s0);

				n0[0] = n1[0] = n2[0] = s0[0];
				n0[1] = n1[1] = n2[1] = s0[1];
				n0[2] = n1[2] = n2[2] = s0[2];
			}

			if (shapes[i].mesh.material_ids.size() > 0) {
				int32_t m = shapes[i].mesh.material_ids[j / 3];

				if (m >= 0) {
					r = materials[m].diffuse[0];
					g = materials[m].diffuse[1];
					b = materials[m].diffuse[2];
				}
			}

			one->vertices[idx * 9 + 0] = p0[0];
			one->vertices[idx * 9 + 1] = p0[1];
			one->vertices[idx * 9 + 2] = p0[2];
			one->vertices[idx * 9 + 3] = p1[0];
			one->vertices[idx * 9 + 4] = p1[1];
			one->vertices[idx * 9 + 5] = p1[2];
			one->vertices[idx * 9 + 6] = p2[0];
			one->vertices[idx * 9 + 7] = p2[1];
			one->vertices[idx * 9 + 8] = p2[2];

			one->normals[idx * 9 + 0] = n0[0];
			one->normals[idx * 9 + 1] = n0[1];
			one->normals[idx * 9 + 2] = n0[2];
			one->normals[idx * 9 + 3] = n1[0];
			one->normals[idx * 9 + 4] = n1[1];
			one->normals[idx * 9 + 5] = n1[2];
			one->normals[idx * 9 + 6] = n2[0];
			one->normals[idx * 9 + 7] = n2[1];
			one->normals[idx * 9 + 8] = n2[2];

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

	max = MAX(maxx - minx, MAX(maxy - miny, maxz - minz));

	nemomatrix_init_identity(&transform);
	nemomatrix_translate_xyz(&transform,
			-(maxx + minx) / 2.0f,
			-(maxy + miny) / 2.0f,
			-(maxz + minz) / 2.0f);
	nemomatrix_scale_xyz(&transform,
			2.0f / max,
			2.0f / max,
			2.0f / max);

	for (i = 0; i < one->count; i++) {
		nemomatrix_transform_xyz(&transform,
				&one->vertices[i * 3 + 0],
				&one->vertices[i * 3 + 1],
				&one->vertices[i * 3 + 2]);
	}

	one->vtransform0.tx = 0.0f;
	one->vtransform0.ty = 0.0f;
	one->vtransform0.tz = 0.0f;
	one->vtransform0.rx = 0.0f;
	one->vtransform0.ry = 0.0f;
	one->vtransform0.rz = 0.0f;
	one->vtransform0.sx = 1.0f;
	one->vtransform0.sy = 1.0f;
	one->vtransform0.sz = 1.0f;

	one->gtransform0.tx = 0.0f;
	one->gtransform0.ty = 0.0f;
	one->gtransform0.tz = 0.0f;
	one->gtransform0.rx = 0.0f;
	one->gtransform0.ry = 0.0f;
	one->gtransform0.rz = 0.0f;
	one->gtransform0.sx = 1.0f;
	one->gtransform0.sy = 1.0f;
	one->gtransform0.sz = 1.0f;

	one->ttransform0.tx = 0.0f;
	one->ttransform0.ty = 0.0f;
	one->ttransform0.r = 0.0f;
	one->ttransform0.sx = 1.0f;
	one->ttransform0.sy = 1.0f;

	one->gtransform.tx = 0.0f;
	one->gtransform.ty = 0.0f;
	one->gtransform.tz = 0.0f;
	one->gtransform.rx = 0.0f;
	one->gtransform.ry = 0.0f;
	one->gtransform.rz = 0.0f;
	one->gtransform.sx = 1.0f;
	one->gtransform.sy = 1.0f;
	one->gtransform.sz = 1.0f;

	one->vtransform.tx = 0.0f;
	one->vtransform.ty = 0.0f;
	one->vtransform.tz = 0.0f;
	one->vtransform.rx = 0.0f;
	one->vtransform.ry = 0.0f;
	one->vtransform.rz = 0.0f;
	one->vtransform.sx = 1.0f;
	one->vtransform.sy = 1.0f;
	one->vtransform.sz = 1.0f;

	one->ttransform.tx = 0.0f;
	one->ttransform.ty = 0.0f;
	one->ttransform.r = 0.0f;
	one->ttransform.sx = 1.0f;
	one->ttransform.sy = 1.0f;

	one->color[0] = 1.0f;
	one->color[1] = 1.0f;
	one->color[2] = 1.0f;
	one->color[3] = 1.0f;

	nemolist_init(&one->link);

	return one;
}
