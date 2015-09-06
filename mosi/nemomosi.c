#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomosi.h>

struct nemomosi *nemomosi_create(int32_t width, int32_t height)
{
	struct nemomosi *mosi;

	mosi = (struct nemomosi *)malloc(sizeof(struct nemomosi));
	if (mosi == NULL)
		return NULL;
	memset(mosi, 0, sizeof(struct nemomosi));

	mosi->width = width;
	mosi->height = height;

	mosi->ones = (struct mosione *)malloc(sizeof(struct mosione) * width * height);
	if (mosi->ones == NULL)
		goto err1;
	memset(mosi->ones, 0, sizeof(struct mosione) * width * height);

	return mosi;

err1:
	free(mosi);

	return NULL;
}

void nemomosi_destroy(struct nemomosi *mosi)
{
	free(mosi->ones);
	free(mosi);
}

void nemomosi_clear_one(struct nemomosi *mosi, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	struct mosione *one;
	int i, j;

	for (i = 0; i < mosi->height; i++) {
		for (j = 0; j < mosi->width; j++) {
			one = &mosi->ones[i * mosi->width + j];

			one->c[3] = a;
			one->c[2] = r;
			one->c[1] = g;
			one->c[0] = b;

			one->has_transition = 0;
		}
	}
}

void nemomosi_tween_color(struct nemomosi *mosi, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	struct mosione *one;
	int i, j;

	for (i = 0; i < mosi->height; i++) {
		for (j = 0; j < mosi->width; j++) {
			one = &mosi->ones[i * mosi->width + j];

			one->c0[3] = one->c[3];
			one->c0[2] = one->c[2];
			one->c0[1] = one->c[1];
			one->c0[0] = one->c[0];

			one->c1[3] = a;
			one->c1[2] = r;
			one->c1[1] = g;
			one->c1[0] = b;
		}
	}
}

void nemomosi_tween_image(struct nemomosi *mosi, uint8_t *c)
{
	struct mosione *one;
	int i, j;

	for (i = 0; i < mosi->height; i++) {
		for (j = 0; j < mosi->width; j++, c += 4) {
			one = &mosi->ones[i * mosi->width + j];

			one->c0[3] = one->c[3];
			one->c0[2] = one->c[2];
			one->c0[1] = one->c[1];
			one->c0[0] = one->c[0];

			one->c1[3] = c[3];
			one->c1[2] = c[2];
			one->c1[1] = c[1];
			one->c1[0] = c[0];
		}
	}
}

int nemomosi_update(struct nemomosi *mosi, uint32_t msecs)
{
	struct mosione *one;
	double t;
	int done = 1;
	int i, j;

	for (i = 0; i < mosi->height; i++) {
		for (j = 0; j < mosi->width; j++) {
			one = &mosi->ones[i * mosi->width + j];

			if (one->has_transition != 0) {
				if (msecs < one->stime) {
					done = 0;
				} else if (msecs >= one->stime && msecs < one->etime) {
					t = ((double)(msecs - one->stime) / (double)(one->etime - one->stime));

					one->c[3] = (one->c1[3] - one->c0[3]) * t + one->c0[3];
					one->c[2] = (one->c1[2] - one->c0[2]) * t + one->c0[2];
					one->c[1] = (one->c1[1] - one->c0[1]) * t + one->c0[1];
					one->c[0] = (one->c1[0] - one->c0[0]) * t + one->c0[0];

					done = 0;
				} else if (msecs >= one->etime) {
					one->c[3] = one->c1[3];
					one->c[2] = one->c1[2];
					one->c[1] = one->c1[1];
					one->c[0] = one->c1[0];

					one->has_transition = 0;
				}
			}
		}
	}

	return done;
}
