#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <float.h>
#include <math.h>

#include <tiny_obj_loader.h>

#include <meshhelper.h>
#include <nemomatrix.h>
#include <nemobox.h>

int mesh_load_triangles(const char *filepath, const char *basepath, float **vertices, float **normals, float **colors)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string r;
	float *_vertices;
	float *_normals;
	float *_colors;
	int _nvertices, _svertices;
	int _nnormals, _snormals;
	int _ncolors, _scolors;
	int i, j;

	r = tinyobj::LoadObj(shapes, materials, filepath, basepath);
	if (!r.empty())
		return 0;

	_vertices = (float *)malloc(sizeof(float) * 16);
	_nvertices = 0;
	_svertices = 16;

	_normals = (float *)malloc(sizeof(float) * 16);
	_nnormals = 0;
	_snormals = 16;

	_colors = (float *)malloc(sizeof(float) * 16);
	_ncolors = 0;
	_scolors = 16;

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
				struct nemovector t0 = { { p0[0], p0[1], p0[2] } };
				struct nemovector t1 = { { p1[0], p1[1], p1[2] } };
				struct nemovector t2 = { { p2[0], p2[1], p2[2] } };
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

			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p0[0]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p0[1]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p0[2]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p1[0]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p1[1]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p1[2]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p2[0]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p2[1]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p2[2]);

			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n0[0]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n0[1]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n0[2]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n1[0]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n1[1]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n1[2]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n2[0]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n2[1]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n2[2]);

			NEMOBOX_APPEND(_colors, _scolors, _ncolors, r);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, g);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, b);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, r);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, g);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, b);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, r);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, g);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, b);
		}
	}

	*vertices = _vertices;
	*normals = _normals;
	*colors = _colors;

	return _nvertices / 9;
}

int mesh_load_lines(const char *filepath, const char *basepath, float **vertices, float **normals, float **colors)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string r;
	float *_vertices;
	float *_normals;
	float *_colors;
	int _nvertices, _svertices;
	int _nnormals, _snormals;
	int _ncolors, _scolors;
	int i, j;

	r = tinyobj::LoadObj(shapes, materials, filepath, basepath);
	if (!r.empty())
		return 0;

	_vertices = (float *)malloc(sizeof(float) * 16);
	_nvertices = 0;
	_svertices = 16;

	_normals = (float *)malloc(sizeof(float) * 16);
	_nnormals = 0;
	_snormals = 16;

	_colors = (float *)malloc(sizeof(float) * 16);
	_ncolors = 0;
	_scolors = 16;

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
				struct nemovector t0 = { { p0[0], p0[1], p0[2] } };
				struct nemovector t1 = { { p1[0], p1[1], p1[2] } };
				struct nemovector t2 = { { p2[0], p2[1], p2[2] } };
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

			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p0[0]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p0[1]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p0[2]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p1[0]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p1[1]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p1[2]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p0[0]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p0[1]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p0[2]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p2[0]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p2[1]);
			NEMOBOX_APPEND(_vertices, _svertices, _nvertices, p2[2]);

			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n0[0]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n0[1]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n0[2]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n1[0]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n1[1]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n1[2]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n0[0]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n0[1]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n0[2]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n2[0]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n2[1]);
			NEMOBOX_APPEND(_normals, _snormals, _nnormals, n2[2]);

			NEMOBOX_APPEND(_colors, _scolors, _ncolors, r);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, g);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, b);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, r);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, g);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, b);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, r);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, g);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, b);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, r);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, g);
			NEMOBOX_APPEND(_colors, _scolors, _ncolors, b);
		}
	}

	*vertices = _vertices;
	*normals = _normals;
	*colors = _colors;

	return _nvertices / 6;
}
