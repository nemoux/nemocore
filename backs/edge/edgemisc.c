#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <edgeback.h>
#include <edgemisc.h>

int edgeback_get_site(struct edgeback *edge, double x, double y)
{
	double dt, db, dl, dr;

	dt = sqrtf(y * y);
	db = sqrtf((edge->height - y) * (edge->height - y));
	dl = sqrtf(x * x);
	dr = sqrtf((edge->width - x) * (edge->width - x));

	if (dt < db && dt < dl && dt < dr)
		return EDGEBACK_TOP_SITE;
	if (dl < dt && dl < db && dl < dr)
		return EDGEBACK_LEFT_SITE;
	if (dr < dt && dr < db && dr < dl)
		return EDGEBACK_RIGHT_SITE;

	return EDGEBACK_BOTTOM_SITE;
}

double edgeback_get_site_rotation(struct edgeback *edge, int site)
{
	if (site == EDGEBACK_TOP_SITE)
		return 180.0f;
	else if (site == EDGEBACK_LEFT_SITE)
		return 90.0f;
	else if (site == EDGEBACK_RIGHT_SITE)
		return -90.0f;

	return 0.0f;
}

int edgeback_get_edge(struct edgeback *edge, double x, double y, uint32_t edgesize)
{
	double dt, db, dl, dr;

	if (edgesize <= 0)
		return EDGEBACK_NONE_SITE;

	dt = sqrtf(y * y);
	db = sqrtf((edge->height - y) * (edge->height - y));
	dl = sqrtf(x * x);
	dr = sqrtf((edge->width - x) * (edge->width - x));

	if (dt < edgesize)
		return EDGEBACK_TOP_SITE;
	if (db < edgesize)
		return EDGEBACK_BOTTOM_SITE;
	if (dl < edgesize)
		return EDGEBACK_LEFT_SITE;
	if (dr < edgesize)
		return EDGEBACK_RIGHT_SITE;

	return EDGEBACK_NONE_SITE;
}
