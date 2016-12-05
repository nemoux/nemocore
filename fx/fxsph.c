#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <fxsph.h>
#include <nemolist.h>

struct sphone {
	float x, y;
	float vx, vy;
	float ax, ay;
	float ex, ey;

	float density;
	float pressure;

	struct nemolist link;
};

struct sphcell {
	struct nemolist list;
};

struct fxsph {
	struct sphone *ones;
	int nones;

	float worldwidth, worldheight;
	float cellsize;
	uint32_t cellwidth, cellheight;

	struct sphcell *cells;
	int ncells;

	float kernel;
	float mass;

	float stiffness;
	float density;
	float walldamping;
	float viscosity;

	float gx, gy;
};

static inline float nemofx_sph_get_length(float x, float y)
{
	return sqrtf(x * x + y * y);
}

static inline float nemofx_sph_get_length_squared(float x, float y)
{
	return x * x + y * y;
}

static inline float nemofx_sph_get_poly6(struct fxsph *sph, float r)
{
	return 315.0f / (64.0f * M_PI * pow(sph->kernel, 9)) * pow(sph->kernel * sph->kernel - r, 3);
}

static inline float nemofx_sph_get_spiky(struct fxsph *sph, float r)
{
	return -45.0f / (M_PI * pow(sph->kernel, 6)) * (sph->kernel - r) * (sph->kernel - r);
}

static inline float nemofx_sph_get_visco(struct fxsph *sph, float r)
{
	return 45.0f / (M_PI * pow(sph->kernel, 6)) * (sph->kernel - r);
}

static inline int nemofx_sph_get_cell_x(struct fxsph *sph, float x)
{
	return (int)(x / sph->cellsize);
}

static inline int nemofx_sph_get_cell_y(struct fxsph *sph, float y)
{
	return (int)(y / sph->cellsize);
}

static inline uint32_t nemofx_sph_get_cell_hash(struct fxsph *sph, uint32_t x, uint32_t y)
{
	if (x < 0.0f || x >= sph->cellwidth || y < 0.0f || y >= sph->cellheight)
		return 0xffffffff;

	return y * sph->cellwidth + x;
}

struct fxsph *nemofx_sph_create(int particles, float width, float height, float kernel, float mass)
{
	struct fxsph *sph;
	int i;

	sph = (struct fxsph *)malloc(sizeof(struct fxsph));
	if (sph == NULL)
		return NULL;
	memset(sph, 0, sizeof(struct fxsph));

	sph->ones = (struct sphone *)malloc(sizeof(struct sphone) * particles);
	if (sph->ones == NULL)
		goto err1;
	memset(sph->ones, 0, sizeof(struct sphone) * particles);

	sph->nones = particles;

	sph->kernel = kernel;
	sph->mass = mass;

	sph->worldwidth = width;
	sph->worldheight = height;
	sph->cellsize = sph->kernel;
	sph->cellwidth = (int)(sph->worldwidth / sph->cellsize);
	sph->cellheight = (int)(sph->worldheight / sph->cellsize);
	sph->ncells = sph->cellwidth * sph->cellheight;

	sph->gx = 0.0f;
	sph->gy = 1.8f;

	sph->stiffness = 1000.0f;
	sph->density = 1000.0f;
	sph->walldamping = 0.0f;
	sph->viscosity = 8.0f;

	sph->cells = (struct sphcell *)malloc(sizeof(struct sphcell) * sph->ncells);
	if (sph->cells == NULL)
		goto err2;
	memset(sph->cells, 0, sizeof(struct sphcell) * sph->ncells);

	for (i = 0; i < sph->nones; i++)
		nemolist_init(&sph->ones[i].link);

	for (i = 0; i < sph->ncells; i++)
		nemolist_init(&sph->cells[i].list);

	return sph;

err2:
	free(sph->ones);

err1:
	free(sph);

	return NULL;
}

void nemofx_sph_destroy(struct fxsph *sph)
{
	free(sph->cells);
	free(sph->ones);

	free(sph);
}

void nemofx_sph_build_cell(struct fxsph *sph)
{
	struct sphone *one;
	uint32_t h;
	int i;

	for (i = 0; i < sph->nones; i++) {
		one = &sph->ones[i];

		h = nemofx_sph_get_cell_hash(sph, nemofx_sph_get_cell_x(sph, one->x), nemofx_sph_get_cell_y(sph, one->y));
		if (h >= sph->ncells)
			continue;

		nemolist_remove(&one->link);
		nemolist_insert(&sph->cells[h].list, &one->link);
	}
}

void nemofx_sph_compute_density_pressure(struct fxsph *sph)
{
	struct sphone *one;
	struct sphone *ione;
	uint32_t h;
	int cx, cy;
	int dx, dy;
	int i;

	for (i = 0; i < sph->nones; i++) {
		one = &sph->ones[i];
		one->density = 0.0f;
		one->pressure = 0.0f;

		cx = nemofx_sph_get_cell_x(sph, one->x);
		cy = nemofx_sph_get_cell_y(sph, one->y);

		for (dy = -1; dy <= 1; dy++) {
			for (dx = -1; dx <= 1; dx++) {
				h = nemofx_sph_get_cell_hash(sph, cx + dx, cy + dy);
				if (h >= sph->ncells)
					continue;

				nemolist_for_each(ione, &sph->cells[h].list, link) {
					float dist2 = nemofx_sph_get_length_squared(ione->x - one->x, ione->y - one->y);

					if (dist2 < 1e-6 || dist2 >= sph->kernel * sph->kernel)
						continue;

					one->density += sph->mass * nemofx_sph_get_poly6(sph, dist2);
				}
			}
		}

		one->density += sph->mass * nemofx_sph_get_poly6(sph, 0.0f);
		one->pressure = (pow(one->density / sph->density, 7) - 1) * sph->stiffness;
	}
}

void nemofx_sph_compute_force(struct fxsph *sph)
{
	struct sphone *one;
	struct sphone *ione;
	uint32_t h;
	int cx, cy;
	int dx, dy;
	int i;

	for (i = 0; i < sph->nones; i++) {
		one = &sph->ones[i];
		one->ax = 0.0f;
		one->ay = 0.0f;

		cx = nemofx_sph_get_cell_x(sph, one->x);
		cy = nemofx_sph_get_cell_y(sph, one->y);

		for (dy = -1; dy <= 1; dy++) {
			for (dx = -1; dx <= 1; dx++) {
				h = nemofx_sph_get_cell_hash(sph, cx + dx, cy + dy);
				if (h >= sph->ncells)
					continue;

				nemolist_for_each(ione, &sph->cells[h].list, link) {
					float dist2 = nemofx_sph_get_length_squared(one->x - ione->x, one->y - ione->y);

					if (dist2 < sph->kernel * sph->kernel && dist2 > 1e-6) {
						float dist = sqrtf(dist2);
						float v = sph->mass / one->density;
						float force0 = v * (one->pressure + ione->pressure) * nemofx_sph_get_spiky(sph, dist);
						float force1 = v * sph->viscosity * nemofx_sph_get_visco(sph, dist);

						one->ax = one->ax - (one->x - ione->x) * force0 / dist;
						one->ay = one->ay - (one->y - ione->y) * force0 / dist;

						one->ax = one->ax + (ione->ex - one->ex) * force1;
						one->ay = one->ay + (ione->ey - one->ey) * force1;
					}
				}
			}
		}

		one->ax = one->ax / one->density + sph->gx;
		one->ay = one->ay / one->density + sph->gy;
	}
}

void nemofx_sph_advect(struct fxsph *sph, float t)
{
	struct sphone *one;
	int i;

	for (i = 0; i < sph->nones; i++) {
		one = &sph->ones[i];

		one->vx = one->vx + one->ax * t;
		one->vy = one->vy + one->ay * t;

		one->x = one->x + one->vx * t;
		one->y = one->y + one->vy * t;

		if (one->x < 0.0f) {
			one->vx = one->vx * sph->walldamping;
			one->x = 0.0f;
		} else if (one->x >= sph->worldwidth) {
			one->vx = one->vx * sph->walldamping;
			one->x = sph->worldwidth - 0.0001f;
		}
		if (one->y < 0.0f) {
			one->vy = one->vy * sph->walldamping;
			one->y = 0.0f;
		} else if (one->y >= sph->worldheight) {
			one->vy = one->vy * sph->walldamping;
			one->y = sph->worldheight - 0.0001f;
		}

		one->ex = (one->ex + one->vx) / 2.0f;
		one->ey = (one->ey + one->vy) / 2.0f;
	}
}

void nemofx_sph_set_particle(struct fxsph *sph, int index, float x, float y, float vx, float vy)
{
	struct sphone *one = &sph->ones[index];

	one->x = x;
	one->y = y;
	one->vx = vx;
	one->vy = vy;
	one->ax = 0.0f;
	one->ay = 0.0f;
	one->ex = vx;
	one->ey = vy;
	one->density = sph->density;

	nemolist_init(&one->link);
}

int nemofx_sph_get_particles(struct fxsph *sph)
{
	return sph->nones;
}

float nemofx_sph_get_particle_x(struct fxsph *sph, int index)
{
	return sph->ones[index].x;
}

float nemofx_sph_get_particle_y(struct fxsph *sph, int index)
{
	return sph->ones[index].y;
}
