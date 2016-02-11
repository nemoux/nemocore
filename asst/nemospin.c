#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemospin.h>

static void nemospin_project_onto_surface(int32_t width, int32_t height, struct nemovector *v)
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

static void nemospin_compute_quaternion(struct nemospin *spin)
{
	struct nemovector v = spin->vec0;
	float dot = nemovector_dot(&spin->vec0, &spin->vec1);
	float angle = acosf(dot);

	if (isnan(angle) == 0) {
		nemovector_cross(&v, &spin->vec1);

		nemoquaternion_make_with_angle_axis(&spin->quat1, angle * 2.0f, v.f[0], v.f[1], v.f[2]);
		nemoquaternion_normalize(&spin->quat1);
		nemoquaternion_multiply(&spin->quat1, &spin->quat0);
	}
}

void nemospin_init(struct nemospin *spin, int32_t width, int32_t height)
{
	nemoquaternion_init_identity(&spin->quat1);

	spin->width = width;
	spin->height = height;
}

void nemospin_resize(struct nemospin *spin, int32_t width, int32_t height)
{
	spin->width = width;
	spin->height = height;
}

void nemospin_reset(struct nemospin *spin, float x, float y)
{
	spin->vec0.f[0] = x;
	spin->vec0.f[1] = y;
	spin->vec0.f[2] = 0.0f;

	nemospin_project_onto_surface(spin->width, spin->height, &spin->vec0);

	spin->quat0 = spin->quat1;
}

void nemospin_update(struct nemospin *spin, float x, float y)
{
	spin->vec1.f[0] = x;
	spin->vec1.f[1] = y;
	spin->vec1.f[2] = 0.0f;

	nemospin_project_onto_surface(spin->width, spin->height, &spin->vec1);
	nemospin_compute_quaternion(spin);
}
