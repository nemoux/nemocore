#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <float.h>
#include <limits.h>
#include <math.h>

#include <nemopath.h>
#include <bezierhelper.h>
#include <nemomisc.h>

struct nemopath *nemopath_create(void)
{
	struct nemopath *path;

	path = (struct nemopath *)malloc(sizeof(struct nemopath));
	if (path == NULL)
		return NULL;
	memset(path, 0, sizeof(struct nemopath));

	path->pathdata = (cairo_path_data_t *)malloc(sizeof(cairo_path_data_t) * 16);
	if (path->pathdata == NULL)
		goto err1;
	path->npathdata = 0;
	path->spathdata = 16;
	path->lastindex = -1;

	path->pathdist = (double *)malloc(sizeof(double) * 16);
	if (path->pathdist == NULL)
		goto err2;
	path->npathdist = 0;
	path->spathdist = 16;

	path->cpath = NULL;
	path->dirty = 0;

	path->index = 0;
	path->offset = 0.0f;

	path->extents[0] = FLT_MAX;
	path->extents[1] = FLT_MAX;
	path->extents[2] = -FLT_MAX;
	path->extents[3] = -FLT_MAX;
	path->length = 0.0f;

	return path;

err2:
	free(path->pathdata);

err1:
	free(path);

	return NULL;
}

void nemopath_destroy(struct nemopath *path)
{
	if (path->cpath != NULL)
		free(path->cpath);

	free(path->pathdist);
	free(path->pathdata);
	free(path);
}

void nemopath_move_to(struct nemopath *path, double x, double y)
{
	cairo_path_data_t data[2];

	data[0].header.type = CAIRO_PATH_MOVE_TO;
	data[0].header.length = 2;
	ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, data[0]);

	data[1].point.x = x;
	data[1].point.y = y;
	ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, data[1]);
	path->lastindex = path->npathdata - 1;

	path->lastdata = data[1];

	path->dirty = 1;
}

void nemopath_line_to(struct nemopath *path, double x, double y)
{
	cairo_path_data_t data[2];

	data[0].header.type = CAIRO_PATH_LINE_TO;
	data[0].header.length = 2;
	ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, data[0]);

	data[1].point.x = x;
	data[1].point.y = y;
	ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, data[1]);

#define	CAIRO_POINT_MINMAX(cp, minx, miny, maxx, maxy) \
	if (cp.point.x < minx) minx = cp.point.x; \
	if (cp.point.y < miny) miny = cp.point.y;	\
	if (cp.point.x > maxx) maxx = cp.point.x;	\
	if (cp.point.y > maxy) maxy = cp.point.y;

	CAIRO_POINT_MINMAX(path->lastdata, path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
	CAIRO_POINT_MINMAX(data[1], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);

	path->lastdata = data[1];

	path->dirty = 1;
}

void nemopath_curve_to(struct nemopath *path, double x1, double y1, double x2, double y2, double x3, double y3)
{
	cairo_path_data_t data[4];

	data[0].header.type = CAIRO_PATH_CURVE_TO;
	data[0].header.length = 4;
	ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, data[0]);

	data[1].point.x = x1;
	data[1].point.y = y1;
	ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, data[1]);

	data[2].point.x = x2;
	data[2].point.y = y2;
	ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, data[2]);

	data[3].point.x = x3;
	data[3].point.y = y3;
	ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, data[3]);

	CAIRO_POINT_MINMAX(path->lastdata, path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
	CAIRO_POINT_MINMAX(data[1], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
	CAIRO_POINT_MINMAX(data[2], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
	CAIRO_POINT_MINMAX(data[3], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);

	path->lastdata = data[3];

	path->dirty = 1;
}

void nemopath_quadratic_curve_to(struct nemopath *path, double x1, double y1, double x2, double y2)
{
	double x0 = path->lastdata.point.x;
	double y0 = path->lastdata.point.y;
	double px3 = x2;
	double py3 = y2;
	double px1 = x0 + 2.0f / 3.0f * (x1 - x0);
	double py1 = y0 + 2.0f / 3.0f * (y1 - y0);
	double px2 = x2 + 2.0f / 3.0f * (x1 - x2);
	double py2 = y2 + 2.0f / 3.0f * (y1 - y2);

	nemopath_curve_to(path, px1, py1, px2, py2, px3, py3);
}

static void nemopath_arc_segment(struct nemopath *path, double xc, double yc, double th0, double th1, double rx, double ry, double x_axis_rotation)
{
	double x1, y1, x2, y2, x3, y3;
	double t;
	double th_half;
	double f, sinf, cosf;

	f = x_axis_rotation * M_PI / 180.0f;
	sinf = sin(f);
	cosf = cos(f);

	th_half = 0.5f * (th1 - th0);
	t = (8.0f / 3.0f) * sin(th_half * 0.5f) * sin(th_half * 0.5f) / sin(th_half);
	x1 = rx * (cos(th0) - t * sin(th0));
	y1 = ry * (sin(th0) + t * cos(th0));
	x3 = rx * cos(th1);
	y3 = ry * sin(th1);
	x2 = x3 + rx * (t * sin(th1));
	y2 = y3 + ry * (-t * cos(th1));

	nemopath_curve_to(path,
			xc + cosf * x1 - sinf * y1,
			yc + sinf * x1 + cosf * y1,
			xc + cosf * x2 - sinf * y2,
			yc + sinf * x2 + cosf * y2,
			xc + cosf * x3 - sinf * y3,
			yc + sinf * x3 + cosf * y3);
}

static void nemopath_arc(struct nemopath *path, double lx, double ly, double rx, double ry, double x_axis_rotation, int large_arc_flag, int sweep_flag, double x, double y)
{
	double f, sinf, cosf;
	double x1, y1, x2, y2;
	double x1_, y1_;
	double cx_, cy_, cx, cy;
	double gamma;
	double theta1, delta_theta;
	double k1, k2, k3, k4, k5;
	int i, nsegs;

	x1 = lx;
	y1 = ly;

	x2 = x;
	y2 = y;

	if (x1 == x2 && y1 == y2)
		return;

	f = x_axis_rotation * M_PI / 180.0f;
	sinf = sin(f);
	cosf = cos(f);

	if ((fabs(rx) < 1e-6) || (fabs(ry) < 1e-6)) {
		nemopath_line_to(path, x, y);
		return;
	}

	if (rx < 0)
		rx = -rx;
	if (ry < 0)
		ry = -ry;

	k1 = (x1 - x2) / 2;
	k2 = (y1 - y2) / 2;

	x1_ = cosf * k1 + sinf * k2;
	y1_ = -sinf * k1 + cosf * k2;

	gamma = (x1_ * x1_) / (rx * rx) + (y1_ * y1_) / (ry * ry);
	if (gamma > 1) {
		rx *= sqrt(gamma);
		ry *= sqrt(gamma);
	}

	k1 = rx * rx * y1_ * y1_ + ry * ry * x1_ * x1_;
	if (k1 == 0)
		return;

	k1 = sqrt(fabs((rx * rx * ry * ry) / k1 - 1));
	if (sweep_flag == large_arc_flag)
		k1 = -k1;

	cx_ = k1 * rx * y1_ / ry;
	cy_ = -k1 * ry * x1_ / rx;

	cx = cosf * cx_ - sinf * cy_ + (x1 + x2) / 2;
	cy = sinf * cx_ + cosf * cy_ + (y1 + y2) / 2;

	k1 = (x1_ - cx_) / rx;
	k2 = (y1_ - cy_) / ry;
	k3 = (-x1_ - cx_) / rx;
	k4 = (-y1_ - cy_) / ry;

	k5 = sqrt(fabs(k1 * k1 + k2 * k2));
	if (k5 == 0)
		return;

	k5 = k1 / k5;
	if (k5 < -1)
		k5 = -1;
	else if (k5 > 1)
		k5 = 1;
	theta1 = acos(k5);
	if (k2 < 0)
		theta1 = -theta1;

	k5 = sqrt(fabs((k1 * k1 + k2 * k2) * (k3 * k3 + k4 * k4)));
	if (k5 == 0)
		return;

	k5 = (k1 * k3 + k2 * k4) / k5;
	if (k5 < -1)
		k5 = -1;
	else if (k5 > 1)
		k5 = 1;
	delta_theta = acos(k5);
	if (k1 * k4 - k3 * k2 < 0)
		delta_theta = -delta_theta;

	if (sweep_flag && delta_theta < 0)
		delta_theta += M_PI * 2;
	else if (!sweep_flag && delta_theta > 0)
		delta_theta -= M_PI * 2;

	nsegs = ceil(fabs(delta_theta / (M_PI * 0.5f + 0.001f)));

	for (i = 0; i < nsegs; i++) {
		nemopath_arc_segment(path, cx, cy,
				theta1 + i * delta_theta / nsegs,
				theta1 + (i + 1) * delta_theta / nsegs,
				rx, ry, x_axis_rotation);
	}
}

void nemopath_arc_to(struct nemopath *path, double cx, double cy, double rx, double ry)
{
	nemopath_arc(path,
			path->lastdata.point.x, path->lastdata.point.y,
			rx, ry,
			0.0f,
			0, 0,
			cx, cy);

	path->lastdata.point.x = cx;
	path->lastdata.point.y = cy;

	path->dirty = 1;
}

void nemopath_close_path(struct nemopath *path)
{
	cairo_path_data_t data;

	data.header.type = CAIRO_PATH_CLOSE_PATH;
	data.header.length = 1;
	ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, data);

	if (path->lastindex >= 0) {
		cairo_path_data_t *moveto = &path->pathdata[path->lastindex];

		CAIRO_POINT_MINMAX(path->lastdata, path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
		CAIRO_POINT_MINMAX(moveto[0], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);

		path->lastdata = moveto[0];
	}

	path->dirty = 1;
}

void nemopath_curve_move_to(struct nemopath *path, double x, double y)
{
	path->cx[2] = path->cx[1] = path->cx[0] = x;
	path->cy[2] = path->cy[1] = path->cy[0] = y;

	nemopath_move_to(path, x, y);
}

void nemopath_curve_line_to(struct nemopath *path, double x, double y)
{
	double sx, sy, ex, ey;

	path->cx[2] = path->cx[1];
	path->cy[2] = path->cy[1];
	path->cx[1] = path->cx[0];
	path->cy[1] = path->cy[0];
	path->cx[0] = x;
	path->cy[0] = y;

	sx = (path->cx[2] + path->cx[1]) * 0.5f;
	sy = (path->cy[2] + path->cy[1]) * 0.5f;
	ex = (path->cx[1] + path->cx[0]) * 0.5f;
	ey = (path->cy[1] + path->cy[0]) * 0.5f;

	nemopath_curve_to(path,
			(sx + 2.0f * path->cx[1]) / 3.0f,
			(sy + 2.0f * path->cy[1]) / 3.0f,
			(ex + 2.0f * path->cx[1]) / 3.0f,
			(ey + 2.0f * path->cy[1]) / 3.0f,
			ex, ey);
}

int nemopath_curve_twist_to(struct nemopath *path, double x, double y, double startradius, double spaceperloop, double starttheta, double endtheta, double thetastep)
{
	double a = startradius;
	double b = spaceperloop;
	double oldtheta = starttheta;
	double newtheta = starttheta;
	double oldr = a + b * oldtheta;
	double newr = a + b * newtheta;
	double nx = 0.0f, ny = 0.0f;
	double oldslope = 0.0f, newslope = 0.0f;
	double ab, cx, cy;
	double oldintercept, newintercept;
	int firstslope = 1;

	nx = x + oldr * cosf(oldtheta);
	ny = y + oldr * sinf(oldtheta);

	nemopath_move_to(path, nx, ny);

	while (oldtheta < endtheta - thetastep) {
		oldtheta = newtheta;
		newtheta += thetastep;

		oldr = newr;
		newr = a + b * newtheta;

		nx = x + newr * cosf(newtheta);
		ny = y + newr * sinf(newtheta);

		ab = a + b * newtheta;
		if (firstslope != 0) {
			oldslope = ((b * sinf(oldtheta) + ab * cosf(oldtheta)) / (b * cosf(oldtheta) - ab * sinf(oldtheta)));
			firstslope = 0;
		} else {
			oldslope = newslope;
		}

		newslope = (b * sinf(newtheta) + ab * cosf(newtheta)) / (b * cosf(newtheta) - ab * sinf(newtheta));

		cx = 0.0f;
		cy = 0.0f;

		oldintercept = -(oldslope * oldr * cosf(oldtheta) - oldr * sinf(oldtheta));
		newintercept = -(newslope * newr * cosf(newtheta) - newr * sinf(newtheta));

		if (oldslope == newslope)
			return -1;

		cx = (newintercept - oldintercept) / (oldslope - newslope);
		cy = oldslope * cx + oldintercept;

		cx += x;
		cy += y;

		nemopath_quadratic_curve_to(path, nx, ny, cx, cy);
	}

	return 0;
}

void nemopath_circle(struct nemopath *path, double cx, double cy, double r)
{
	nemopath_move_to(path, cx + r, cy);
	nemopath_curve_to(path,
			cx + r,
			cy + r * NEMOPATH_ARC_MAGIC,
			cx + r * NEMOPATH_ARC_MAGIC,
			cy + r,
			cx,
			cy + r);
	nemopath_curve_to(path,
			cx - r * NEMOPATH_ARC_MAGIC,
			cy + r,
			cx - r,
			cy + r * NEMOPATH_ARC_MAGIC,
			cx - r,
			cy);
	nemopath_curve_to(path,
			cx - r,
			cy - r * NEMOPATH_ARC_MAGIC,
			cx - r * NEMOPATH_ARC_MAGIC,
			cy - r,
			cx,
			cy - r);
	nemopath_curve_to(path,
			cx + r * NEMOPATH_ARC_MAGIC,
			cy - r,
			cx + r,
			cy - r * NEMOPATH_ARC_MAGIC,
			cx + r,
			cy);
}

void nemopath_rect(struct nemopath *path, double x, double y, double w, double h)
{
	nemopath_move_to(path, x, y);
	nemopath_line_to(path, x + w, y);
	nemopath_line_to(path, x + w, y + h);
	nemopath_line_to(path, x, y + h);
	nemopath_close_path(path);
}

int nemopath_append_path(struct nemopath *path, cairo_path_t *cpath)
{
	cairo_path_data_t cp, *data, *lp;
	int i;

	if (cpath == NULL || cpath->data == NULL)
		return -1;

	for (i = 0; i < cpath->num_data; i += cpath->data[i].header.length) {
		data = &cpath->data[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
				cp.header.type = CAIRO_PATH_MOVE_TO;
				cp.header.length = 2;
				ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, cp);
				cp.point.x = data[1].point.x;
				cp.point.y = data[1].point.y;
				ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, cp);
				path->lastindex = path->npathdata - 1;

				path->lastdata = cp;
				break;

			case CAIRO_PATH_CLOSE_PATH:
				cp.header.type = CAIRO_PATH_CLOSE_PATH;
				cp.header.length = 1;
				ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, cp);

				if (path->lastindex >= 0) {
					lp = &path->pathdata[path->lastindex];

					cp.header.type = CAIRO_PATH_MOVE_TO;
					cp.header.length = 2;
					ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, cp);
					cp.point.x = lp->point.x;
					cp.point.y = lp->point.y;
					ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, cp);
					path->lastindex = path->npathdata - 1;

					CAIRO_POINT_MINMAX(path->lastdata, path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
					CAIRO_POINT_MINMAX(cp, path->extents[0], path->extents[1], path->extents[2], path->extents[3]);

					path->lastdata = cp;
				}
				break;

			case CAIRO_PATH_LINE_TO:
				cp.header.type = CAIRO_PATH_LINE_TO;
				cp.header.length = 2;
				ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, cp);
				cp.point.x = data[1].point.x;
				cp.point.y = data[1].point.y;
				ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, cp);

				CAIRO_POINT_MINMAX(path->lastdata, path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
				CAIRO_POINT_MINMAX(data[1], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);

				path->lastdata = cp;
				break;

			case CAIRO_PATH_CURVE_TO:
				cp.header.type = CAIRO_PATH_CURVE_TO;
				cp.header.length = 4;
				ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, cp);
				cp.point.x = data[1].point.x;
				cp.point.y = data[1].point.y;
				ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, cp);
				cp.point.x = data[2].point.x;
				cp.point.y = data[2].point.y;
				ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, cp);
				cp.point.x = data[3].point.x;
				cp.point.y = data[3].point.y;
				ARRAY_APPEND(path->pathdata, path->spathdata, path->npathdata, cp);

				CAIRO_POINT_MINMAX(path->lastdata, path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
				CAIRO_POINT_MINMAX(data[1], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
				CAIRO_POINT_MINMAX(data[2], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
				CAIRO_POINT_MINMAX(data[3], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);

				path->lastdata = data[3];
				break;

			default:
				break;
		}
	}

	path->dirty = 1;

	return 0;
}

struct pathcontext {
	struct nemopath *path;

	cairo_path_data_t cp;
	cairo_path_data_t rp;

	char cmd;
	int param;
	int relative;
	double params[7];
};

static void nemopath_cmd_default_xy(struct pathcontext *context, int nparams)
{
	int i;

	if (context->relative) {
		for (i = context->param; i < nparams; i++) {
			if (i > 2)
				context->params[i] = context->params[i - 2];
			else if (i == 1)
				context->params[i] = context->cp.point.y;
			else if (i == 0)
				context->params[i] = context->cp.point.x;
		}
	} else {
		for (i = context->param; i < nparams; i++)
			context->params[i] = 0.0f;
	}
}

static void nemopath_cmd_flush(struct pathcontext *context, int final)
{
	double x1, y1, x2, y2, x3, y3;

	switch (context->cmd) {
		case 'm':
			if (context->param == 2 || final) {
				nemopath_cmd_default_xy(context, 2);
				nemopath_move_to(context->path, context->params[0], context->params[1]);
				context->cp.point.x = context->rp.point.x = context->params[0];
				context->cp.point.y = context->rp.point.y = context->params[1];
				context->param = 0;
				context->cmd = 'l';
			}
			break;

		case 'l':
			if (context->param == 2 || final) {
				nemopath_cmd_default_xy(context, 2);
				nemopath_line_to(context->path, context->params[0], context->params[1]);
				context->cp.point.x = context->rp.point.x = context->params[0];
				context->cp.point.y = context->rp.point.y = context->params[1];
				context->param = 0;
			}
			break;

		case 'c':
			if (context->param == 6 || final) {
				nemopath_cmd_default_xy(context, 6);
				x1 = context->params[0];
				y1 = context->params[1];
				x2 = context->params[2];
				y2 = context->params[3];
				x3 = context->params[4];
				y3 = context->params[5];
				nemopath_curve_to(context->path, x1, y1, x2, y2, x3, y3);
				context->rp.point.x = x2;
				context->rp.point.y = y2;
				context->cp.point.x = x3;
				context->cp.point.y = y3;
				context->param = 0;
			}
			break;

		case 's':
			if (context->param == 4 || final) {
				nemopath_cmd_default_xy(context, 4);
				x1 = 2 * context->cp.point.x - context->rp.point.x;
				y1 = 2 * context->cp.point.y - context->rp.point.y;
				x2 = context->params[0];
				y2 = context->params[1];
				x3 = context->params[2];
				y3 = context->params[3];
				nemopath_curve_to(context->path, x1, y1, x2, y2, x3, y3);
				context->rp.point.x = x2;
				context->rp.point.y = y2;
				context->cp.point.x = x3;
				context->cp.point.y = y3;
				context->param = 0;
			}
			break;

		case 'h':
			if (context->param == 1) {
				nemopath_line_to(context->path, context->params[0], context->cp.point.y);
				context->cp.point.x = context->rp.point.x = context->params[0];
				context->param = 0;
			}
			break;

		case 'v':
			if (context->param == 1) {
				nemopath_line_to(context->path, context->cp.point.x, context->params[0]);
				context->cp.point.y = context->rp.point.y = context->params[0];
				context->param = 0;
			}
			break;

		case 'q':
			if (context->param == 4 || final) {
				nemopath_cmd_default_xy(context, 4);
				x1 = (context->cp.point.x + 2 * context->params[0]) * (1.0f / 3.0f);
				y1 = (context->cp.point.y + 2 * context->params[1]) * (1.0f / 3.0f);
				x3 = context->params[2];
				y3 = context->params[3];
				x2 = (x3 + 2 * context->params[0]) * (1.0f / 3.0f);
				y2 = (y3 + 2 * context->params[1]) * (1.0f / 3.0f);
				nemopath_curve_to(context->path, x1, y1, x2, y2, x3, y3);
				context->rp.point.x = context->params[0];
				context->rp.point.y = context->params[1];
				context->cp.point.x = x3;
				context->cp.point.y = y3;
				context->param = 0;
			}
			break;

		case 't':
			if (context->param == 2 || final) {
				double xc, yc;

				xc = 2 * context->cp.point.x - context->rp.point.x;
				yc = 2 * context->cp.point.y - context->rp.point.y;
				x1 = (context->cp.point.x + 2 * xc) * (1.0f / 3.0f);
				y1 = (context->cp.point.y + 2 * yc) * (1.0f / 3.0f);
				x3 = context->params[0];
				y3 = context->params[1];
				x2 = (x3 + 2 * xc) * (1.0f / 3.0f);
				y2 = (y3 + 2 * yc) * (1.0f / 3.0f);
				nemopath_curve_to(context->path, x1, y1, x2, y2, x3, y3);
				context->rp.point.x = xc;
				context->rp.point.y = yc;
				context->cp.point.x = x3;
				context->cp.point.y = y3;
				context->param = 0;
			} else if (final) {
				if (context->param > 2) {
					nemopath_cmd_default_xy(context, 4);
					x1 = (context->cp.point.x + 2 * context->params[0]) * (1.0f / 3.0f);
					y1 = (context->cp.point.y + 2 * context->params[1]) * (1.0f / 3.0f);
					x3 = context->params[2];
					y3 = context->params[3];
					x2 = (x3 + 2 * context->params[0]) * (1.0f / 3.0f);
					y2 = (y3 + 2 * context->params[1]) * (1.0f / 3.0f);
					nemopath_curve_to(context->path, x1, y1, x2, y2, x3, y3);
					context->rp.point.x = context->params[0];
					context->rp.point.y = context->params[1];
					context->cp.point.x = x3;
					context->cp.point.y = y3;
				} else {
					nemopath_cmd_default_xy(context, 2);
					nemopath_line_to(context->path, context->params[0], context->params[1]);
					context->cp.point.x = context->rp.point.x = context->params[0];
					context->cp.point.y = context->rp.point.y = context->params[1];
				}
				context->param = 0;
			}
			break;

		case 'a':
			if (context->param == 7 || final) {
				nemopath_arc(context->path,
						context->cp.point.x, context->cp.point.y,
						context->params[0], context->params[1], context->params[2],
						context->params[3], context->params[4], context->params[5], context->params[6]);
				context->cp.point.x = context->params[5];
				context->cp.point.y = context->params[6];
				context->param = 0;
			}
			break;

		default:
			context->param = 0;
	}
}

static void nemopath_cmd_end_of_number(struct pathcontext *context, double val, int sign, int exp_sign, int exp)
{
	val *= sign * pow(10, exp_sign * exp);

	if (context->relative) {
		switch (context->cmd) {
			case 'l':
			case 'm':
			case 'c':
			case 's':
			case 'q':
			case 't':
				if ((context->param & 1) == 0)
					val += context->cp.point.x;
				else if ((context->param & 1) == 1)
					val += context->cp.point.y;
				break;

			case 'a':
				if (context->param == 5)
					val += context->cp.point.x;
				else if (context->param == 6)
					val += context->cp.point.y;
				break;

			case 'h':
				val += context->cp.point.x;
				break;

			case 'v':
				val += context->cp.point.y;
				break;
		}
	}

	context->params[context->param++] = val;

	nemopath_cmd_flush(context, 0);
}

static void nemopath_cmd_parse(struct pathcontext *context, const char *data)
{
	int i = 0;
	double val = 0.0f;
	char c = 0;
	int in_num = 0;
	int in_frac = 0;
	int in_exp = 0;
	int exp_wait_sign = 0;
	int sign = 0;
	int exp = 0;
	int exp_sign = 0;
	double frac = 0.0f;

	in_num = 0;
	for (i = 0; ; i++) {
		c = data[i];
		if (c >= '0' && c <= '9') {
			if (in_num) {
				if (in_exp) {
					exp = (exp * 10) + c - '0';
					exp_wait_sign = 0;
				} else if (in_frac) {
					val += (frac *= 0.1) * (c - '0');
				} else {
					val = (val * 10) + c - '0';
				}
			} else {
				in_num = 1;
				in_frac = 0;
				in_exp = 0;
				exp = 0;
				exp_sign = 1;
				exp_wait_sign = 0;
				val = c - '0';
				sign = 1;
			}
		} else if (c == '.') {
			if (!in_num) {
				in_frac = 1;
				val = 0;
			} else if (in_frac) {
				nemopath_cmd_end_of_number(context, val, sign, exp_sign, exp);
				in_frac = 0;
				in_exp = 0;
				exp = 0;
				exp_sign = 1;
				exp_wait_sign = 0;
				val = 0;
				sign = 1;
			} else {
				in_frac = 1;
			}
			in_num = 1;
			frac = 1;
		} else if ((c == 'E' || c == 'e') && in_num) {
			in_exp = 1;
			exp_wait_sign = 1;
			exp = 0;
			exp_sign = 1;
		} else if ((c == '+' || c == '-') && in_exp) {
			exp_sign = c == '+' ? 1 : -1;
		} else if (in_num) {
			nemopath_cmd_end_of_number(context, val, sign, exp_sign, exp);
			in_num = 0;
		}

		if (c == '\0') {
			break;
		} else if ((c == '+' || c == '-') && !exp_wait_sign) {
			sign = c == '+' ? 1 : -1;
			val = 0;
			in_num = 1;
			in_frac = 0;
			in_exp = 0;
			exp = 0;
			exp_sign = 1;
			exp_wait_sign = 0;
		} else if (c == 'z' || c == 'Z') {
			if (context->param)
				nemopath_cmd_flush(context, 1);
			nemopath_close_path(context->path);

			context->cp = context->rp = context->path->pathdata[context->path->npathdata - 1];
		} else if (c >= 'A' && c <= 'Z' && c != 'E') {
			if (context->param)
				nemopath_cmd_flush(context, 1);
			context->cmd = c + 'a' - 'A';
			context->relative = 0;
		} else if (c >= 'a' && c <= 'z' && c != 'e') {
			if (context->param)
				nemopath_cmd_flush(context, 1);
			context->cmd = c;
			context->relative = 1;
		}
	}

	if (context->param)
		nemopath_cmd_flush(context, 1);
}

int nemopath_append_cmd(struct nemopath *path, const char *cmd)
{
	struct pathcontext context;

	context.path = path;

	context.cp.point.x = 0.0f;
	context.cp.point.y = 0.0f;
	context.cmd = 0;
	context.param = 0;

	nemopath_cmd_parse(&context, cmd);

	return 0;
}

void nemopath_clear_path(struct nemopath *path)
{
	path->npathdata = 0;
	path->lastindex = -1;

	path->dirty = 1;

	path->extents[0] = FLT_MAX;
	path->extents[1] = FLT_MAX;
	path->extents[2] = -FLT_MAX;
	path->extents[3] = -FLT_MAX;
}

cairo_path_t *nemopath_get_cairo_path(struct nemopath *path)
{
	if (path->dirty != 0) {
		if (path->cpath != NULL) {
			free(path->cpath);

			path->cpath = NULL;
		}

		path->dirty = 0;
	}

	if (path->cpath == NULL) {
		cairo_path_t *cpath;

		cpath = (cairo_path_t *)malloc(sizeof(cairo_path_t));
		if (cpath == NULL)
			return NULL;

		cpath->status = CAIRO_STATUS_SUCCESS;
		cpath->data = path->pathdata;
		cpath->num_data = path->npathdata;

		path->cpath = cpath;
	}

	return path->cpath;
}

int nemopath_draw_all(struct nemopath *path, cairo_t *cr, double *extents)
{
	cairo_path_t *cpath = nemopath_get_cairo_path(path);

	cairo_append_path(cr, cpath);

	extents[0] = path->extents[0];
	extents[1] = path->extents[1];
	extents[2] = path->extents[2];
	extents[3] = path->extents[3];

	return 0;
}

int nemopath_draw_subpath(struct nemopath *path, cairo_t *cr, double *extents)
{
	cairo_path_data_t *data, lp, cp;
	int i;

	extents[0] = FLT_MAX;
	extents[1] = FLT_MAX;
	extents[2] = -FLT_MAX;
	extents[3] = -FLT_MAX;

	if (path->index > 0) {
		cp = path->cp;

		cairo_move_to(cr, cp.point.x, cp.point.y);
	}

	for (i = path->index; i < path->npathdata; i += path->pathdata[i].header.length) {
		data = &path->pathdata[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
				cairo_move_to(cr, data[1].point.x, data[1].point.y);

				lp = data[1];
				cp = data[1];
				break;

			case CAIRO_PATH_CLOSE_PATH:
				data = (&lp) - 1;
			case CAIRO_PATH_LINE_TO:
				cairo_line_to(cr, data[1].point.x, data[1].point.y);

				CAIRO_POINT_MINMAX(cp, extents[0], extents[1], extents[2], extents[3]);
				CAIRO_POINT_MINMAX(data[1], extents[0], extents[1], extents[2], extents[3]);

				cp = data[1];
				break;

			case CAIRO_PATH_CURVE_TO:
				cairo_curve_to(cr,
						data[1].point.x, data[1].point.y,
						data[2].point.x, data[2].point.y,
						data[3].point.x, data[3].point.y);

				CAIRO_POINT_MINMAX(cp, extents[0], extents[1], extents[2], extents[3]);
				CAIRO_POINT_MINMAX(data[1], extents[0], extents[1], extents[2], extents[3]);
				CAIRO_POINT_MINMAX(data[2], extents[0], extents[1], extents[2], extents[3]);
				CAIRO_POINT_MINMAX(data[3], extents[0], extents[1], extents[2], extents[3]);

				cp = data[3];
				break;

			default:
				return -1;
		}
	}

	path->cp = cp;
	path->index = i;

	return 0;
}

int nemopath_extents_subpath(struct nemopath *path, double *extents)
{
	cairo_path_data_t *data, lp, cp;
	int i;

	extents[0] = FLT_MAX;
	extents[1] = FLT_MAX;
	extents[2] = -FLT_MAX;
	extents[3] = -FLT_MAX;

	if (path->index > 0) {
		cp = path->cp;
	}

	for (i = path->index; i < path->npathdata; i += path->pathdata[i].header.length) {
		data = &path->pathdata[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
				lp = data[1];
				cp = data[1];
				break;

			case CAIRO_PATH_CLOSE_PATH:
				data = (&lp) - 1;
			case CAIRO_PATH_LINE_TO:
				CAIRO_POINT_MINMAX(cp, extents[0], extents[1], extents[2], extents[3]);
				CAIRO_POINT_MINMAX(data[1], extents[0], extents[1], extents[2], extents[3]);

				cp = data[1];
				break;

			case CAIRO_PATH_CURVE_TO:
				CAIRO_POINT_MINMAX(cp, extents[0], extents[1], extents[2], extents[3]);
				CAIRO_POINT_MINMAX(data[1], extents[0], extents[1], extents[2], extents[3]);
				CAIRO_POINT_MINMAX(data[2], extents[0], extents[1], extents[2], extents[3]);
				CAIRO_POINT_MINMAX(data[3], extents[0], extents[1], extents[2], extents[3]);

				cp = data[3];
				break;

			default:
				return -1;
		}
	}

	path->cp = cp;
	path->index = i;

	return 0;
}

void nemopath_translate(struct nemopath *path, double x, double y)
{
	cairo_path_data_t *data;
	int i;

	for (i = 0; i < path->npathdata; i += path->pathdata[i].header.length) {
		data = &path->pathdata[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
				data[1].point.x += x;
				data[1].point.y += y;
				break;

			case CAIRO_PATH_CLOSE_PATH:
				break;

			case CAIRO_PATH_LINE_TO:
				data[1].point.x += x;
				data[1].point.y += y;
				break;

			case CAIRO_PATH_CURVE_TO:
				data[1].point.x += x;
				data[1].point.y += y;
				data[2].point.x += x;
				data[2].point.y += y;
				data[3].point.x += x;
				data[3].point.y += y;
				break;

			default:
				break;
		}
	}

	path->extents[0] += x;
	path->extents[1] += y;
	path->extents[2] += x;
	path->extents[3] += y;
}

void nemopath_scale(struct nemopath *path, double sx, double sy)
{
	cairo_path_data_t *data;
	int i;

	for (i = 0; i < path->npathdata; i += path->pathdata[i].header.length) {
		data = &path->pathdata[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
				data[1].point.x *= sx;
				data[1].point.y *= sy;
				break;

			case CAIRO_PATH_CLOSE_PATH:
				break;

			case CAIRO_PATH_LINE_TO:
				data[1].point.x *= sx;
				data[1].point.y *= sy;
				break;

			case CAIRO_PATH_CURVE_TO:
				data[1].point.x *= sx;
				data[1].point.y *= sy;
				data[2].point.x *= sx;
				data[2].point.y *= sy;
				data[3].point.x *= sx;
				data[3].point.y *= sy;
				break;

			default:
				break;
		}
	}

	path->extents[0] *= sx;
	path->extents[1] *= sy;
	path->extents[2] *= sx;
	path->extents[3] *= sy;
}

void nemopath_transform(struct nemopath *path, cairo_matrix_t *matrix)
{
	cairo_path_data_t *data;
	int i;

	for (i = 0; i < path->npathdata; i += path->pathdata[i].header.length) {
		data = &path->pathdata[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
				cairo_matrix_transform_point(matrix,
						&data[1].point.x,
						&data[1].point.y);
				break;

			case CAIRO_PATH_CLOSE_PATH:
				break;

			case CAIRO_PATH_LINE_TO:
				cairo_matrix_transform_point(matrix,
						&data[1].point.x,
						&data[1].point.y);
				break;

			case CAIRO_PATH_CURVE_TO:
				cairo_matrix_transform_point(matrix,
						&data[1].point.x,
						&data[1].point.y);
				cairo_matrix_transform_point(matrix,
						&data[2].point.x,
						&data[2].point.y);
				cairo_matrix_transform_point(matrix,
						&data[3].point.x,
						&data[3].point.y);
				break;

			default:
				break;
		}
	}

	cairo_matrix_transform_point(matrix, &path->extents[0], &path->extents[1]);
	cairo_matrix_transform_point(matrix, &path->extents[2], &path->extents[3]);
}

static void nemopath_update_length(struct nemopath *path)
{
	cairo_path_data_t *data, lp, cp;
	double l;
	int i;

#define	CAIRO_PATH_DISTANCE(a, b)	\
	sqrtf((b.point.x - a.point.x) * (b.point.x - a.point.x) + (b.point.y - a.point.y) * (b.point.y - a.point.y))

	for (i = path->npathdist; i < path->npathdata; i += path->pathdata[i].header.length) {
		data = &path->pathdata[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
				l = 0.0f;
				lp = data[1];
				cp = data[1];
				ARRAY_APPEND(path->pathdist, path->spathdist, path->npathdist, 0.0f);
				ARRAY_APPEND(path->pathdist, path->spathdist, path->npathdist, 0.0f);
				break;

			case CAIRO_PATH_CLOSE_PATH:
				data = (&lp) - 1;
			case CAIRO_PATH_LINE_TO:
				l = CAIRO_PATH_DISTANCE(cp, data[1]);
				cp = data[1];
				ARRAY_APPEND(path->pathdist, path->spathdist, path->npathdist, l);
				ARRAY_APPEND(path->pathdist, path->spathdist, path->npathdist, 0.0f);
				break;

			case CAIRO_PATH_CURVE_TO:
				l = cubicbezier_length(
						cp.point.x, cp.point.y,
						data[1].point.x, data[1].point.y,
						data[2].point.x, data[2].point.y,
						data[3].point.x, data[3].point.y,
						NEMOPATH_CUBIC_BEZIER_FLATTEN_STEPS);
				cp = data[3];
				ARRAY_APPEND(path->pathdist, path->spathdist, path->npathdist, l);
				ARRAY_APPEND(path->pathdist, path->spathdist, path->npathdist, 0.0f);
				ARRAY_APPEND(path->pathdist, path->spathdist, path->npathdist, 0.0f);
				ARRAY_APPEND(path->pathdist, path->spathdist, path->npathdist, 0.0f);
				break;

			default:
				break;
		}

		path->length += l;
	}
}

static void nemopath_update_extents(struct nemopath *path)
{
	cairo_path_data_t *data, lp, cp;
	int i;

	path->extents[0] = FLT_MAX;
	path->extents[1] = FLT_MAX;
	path->extents[2] = -FLT_MAX;
	path->extents[3] = -FLT_MAX;

	path->length = 0.0f;
	path->npathdist = 0;

	for (i = 0; i < path->npathdata; i += path->pathdata[i].header.length) {
		data = &path->pathdata[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
				lp = data[1];
				cp = data[1];
				break;

			case CAIRO_PATH_CLOSE_PATH:
				data = (&lp) - 1;
			case CAIRO_PATH_LINE_TO:
				CAIRO_POINT_MINMAX(cp, path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
				CAIRO_POINT_MINMAX(data[1], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);

				cp = data[1];
				break;

			case CAIRO_PATH_CURVE_TO:
				CAIRO_POINT_MINMAX(cp, path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
				CAIRO_POINT_MINMAX(data[1], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
				CAIRO_POINT_MINMAX(data[2], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
				CAIRO_POINT_MINMAX(data[3], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);

				cp = data[3];
				break;

			default:
				break;
		}
	}
}

double nemopath_get_position(struct nemopath *path, double offset, double *px, double *py)
{
	cairo_path_data_t *data, lp, cp;
	double p, l;
	double ox, oy, pr = 0.0f;
	int i;

	if (path->npathdist != path->npathdata)
		nemopath_update_length(path);

	if (path->offset <= offset) {
		offset = offset - path->offset;
		lp = path->lp;
		cp = path->cp;
	} else {
		path->index = 0;
		path->offset = 0.0f;
	}

	for (i = path->index; i < path->npathdata && offset >= 0.0f; i += path->pathdata[i].header.length) {
		data = &path->pathdata[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
				l = 0.0f;
				lp = data[1];
				cp = data[1];
				path->lp = data[1];
				path->cp = data[1];
				break;

			case CAIRO_PATH_CLOSE_PATH:
				data = (&lp) - 1;
			case CAIRO_PATH_LINE_TO:
				l = path->pathdist[i];
				if (l <= 0.0f)
					break;
				if (offset > l) {
					path->index = i + path->pathdata[i].header.length;
					path->offset += l;
					path->cp = data[1];
					p = 1.0f;
				} else {
					p = offset / l;
				}
				*px = (data[1].point.x - cp.point.x) * p + cp.point.x;
				*py = (data[1].point.y - cp.point.y) * p + cp.point.y;
				pr = atan2(cp.point.x - data[1].point.x, -(cp.point.y - data[1].point.y));
				cp = data[1];
				break;

			case CAIRO_PATH_CURVE_TO:
				l = path->pathdist[i];
				if (offset > l) {
					path->index = i + path->pathdata[i].header.length;
					path->offset += l;
					path->cp = data[3];
					p = 1.0f;
				} else {
					p = offset / l;
				}
				*px = cubicbezier_point(p,
						cp.point.x,
						data[1].point.x,
						data[2].point.x,
						data[3].point.x);
				*py = cubicbezier_point(p,
						cp.point.y,
						data[1].point.y,
						data[2].point.y,
						data[3].point.y);
				ox = cubicbezier_point(p - (double)NEMOPATH_CUBIC_BEZIER_FLATTEN_GAPS / l,
						cp.point.x,
						data[1].point.x,
						data[2].point.x,
						data[3].point.x);
				oy = cubicbezier_point(p - (double)NEMOPATH_CUBIC_BEZIER_FLATTEN_GAPS / l,
						cp.point.y,
						data[1].point.y,
						data[2].point.y,
						data[3].point.y);
				pr = atan2(ox - *px, -(oy - *py));
				cp = data[3];
				break;

			default:
				break;
		}

		offset -= l;
	}

	return pr;
}

double nemopath_get_progress(struct nemopath *path, double start, double end, double steps, double x, double y)
{
	double px, py;
	double dx, dy;
	double dist;
	double mindist = FLT_MAX;
	double min = -1.0f;
	double offset, p;

	if (path->npathdist != path->npathdata)
		nemopath_update_length(path);

	for (p = start; p <= end; p = p + steps) {
		if (p < 0.0f)
			offset = p + path->length;
		else if (p > path->length)
			offset = p - path->length;
		else
			offset = p;

		nemopath_get_position(path, offset, &px, &py);

		dx = x - px;
		dy = y - py;

		dist = sqrtf(dx * dx + dy * dy);
		if (dist < mindist) {
			mindist = dist;
			min = offset;
		}
	}

	return min;
}

struct nemopath *nemopath_clone(struct nemopath *path)
{
	struct nemopath *dpath;

	dpath = (struct nemopath *)malloc(sizeof(struct nemopath));
	if (dpath == NULL)
		return NULL;
	memset(dpath, 0, sizeof(struct nemopath));

	dpath->pathdata = (cairo_path_data_t *)malloc(sizeof(cairo_path_data_t) * path->spathdata);
	if (dpath->pathdata == NULL)
		goto err1;
	memcpy(dpath->pathdata, path->pathdata, sizeof(cairo_path_data_t) * path->spathdata);

	dpath->npathdata = path->npathdata;
	dpath->spathdata = path->spathdata;
	dpath->lastindex = -1;

	dpath->pathdist = (double *)malloc(sizeof(double) * path->spathdist);
	if (dpath->pathdist == NULL)
		goto err2;
	memcpy(dpath->pathdist, path->pathdist, sizeof(double) * path->spathdist);

	dpath->npathdist = path->npathdist;
	dpath->spathdist = path->spathdist;

	dpath->cpath = NULL;
	dpath->dirty = 0;

	dpath->index = 0;
	dpath->offset = 0.0f;

	dpath->extents[0] = path->extents[0];
	dpath->extents[1] = path->extents[1];
	dpath->extents[2] = path->extents[2];
	dpath->extents[3] = path->extents[3];
	dpath->length = path->length;

	return dpath;

err2:
	free(dpath->pathdata);

err1:
	free(dpath);

	return NULL;
}

int nemopath_flatten(struct nemopath *path)
{
	cairo_path_data_t *pathdata;
	cairo_path_data_t *data;
	double length;
	int npathdata;
	int i;

	pathdata = path->pathdata;
	npathdata = path->npathdata;

	path->pathdata = (cairo_path_data_t *)malloc(sizeof(cairo_path_data_t) * 32);
	if (path->pathdata == NULL)
		return -1;
	path->npathdata = 0;
	path->spathdata = 32;
	path->npathdist = 0;
	path->lastindex = -1;
	path->dirty = 1;

	path->extents[0] = FLT_MAX;
	path->extents[1] = FLT_MAX;
	path->extents[2] = -FLT_MAX;
	path->extents[3] = -FLT_MAX;

	for (i = 0; i < npathdata; i += pathdata[i].header.length) {
		data = &pathdata[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
				nemopath_move_to(path, data[1].point.x, data[1].point.y);
				break;

			case CAIRO_PATH_CLOSE_PATH:
				nemopath_close_path(path);
				break;

			case CAIRO_PATH_LINE_TO:
				nemopath_line_to(path, data[1].point.x, data[1].point.y);
				break;

			case CAIRO_PATH_CURVE_TO:
				length = cubicbezier_length(
						path->lastdata.point.x, path->lastdata.point.y,
						data[1].point.x, data[1].point.y,
						data[2].point.x, data[2].point.y,
						data[3].point.x, data[3].point.y,
						NEMOPATH_CUBIC_BEZIER_FLATTEN_STEPS);

				if (length > 0.0f) {
					double x0 = path->lastdata.point.x;
					double y0 = path->lastdata.point.y;
					double x1 = data[1].point.x;
					double y1 = data[1].point.y;
					double x2 = data[2].point.x;
					double y2 = data[2].point.y;
					double x3 = data[3].point.x;
					double y3 = data[3].point.y;
					double t, cx, cy;
					int steps = ceil(length / NEMOPATH_CUBIC_BEZIER_FLATTEN_GAPS);
					int s;

					for (s = 1; s <= steps; s++) {
						t = (double)s / (double)steps;

						cx = cubicbezier_point(t, x0, x1, x2, x3);
						cy = cubicbezier_point(t, y0, y1, y2, y3);

						nemopath_line_to(path, cx, cy);
					}
				}
				break;

			default:
				break;
		}
	}

	free(pathdata);

	return 0;
}

int nemopath_split(struct nemopath *path, int elements)
{
	cairo_path_data_t *pathdata;
	cairo_path_data_t *data;
	cairo_path_data_t cp, lp;
	double *pathdist;
	double length;
	double l;
	int npathdata;
	int count;
	int remain;
	int i, j;

	if (path->npathdist != path->npathdata)
		nemopath_update_length(path);

	length = nemopath_length(path);
	remain = nemopath_elements(path);

	pathdata = path->pathdata;
	npathdata = path->npathdata;

	pathdist = path->pathdist;

	path->pathdata = (cairo_path_data_t *)malloc(sizeof(cairo_path_data_t) * 32);
	if (path->pathdata == NULL)
		return -1;
	path->pathdist = (double *)malloc(sizeof(double) * 16);
	if (path->pathdist == NULL)
		return -1;
	path->npathdata = 0;
	path->spathdata = 32;
	path->npathdist = 0;
	path->spathdist = 16;
	path->lastindex = -1;
	path->dirty = 1;

	path->extents[0] = FLT_MAX;
	path->extents[1] = FLT_MAX;
	path->extents[2] = -FLT_MAX;
	path->extents[3] = -FLT_MAX;

	for (i = 0; i < npathdata; i += pathdata[i].header.length) {
		data = &pathdata[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
				l = 0.0f;

				nemopath_move_to(path,
						data[1].point.x,
						data[1].point.y);

				lp = data[1];
				cp = data[1];

				elements -= 1;
				break;

			case CAIRO_PATH_CLOSE_PATH:
				data = (&lp) - 1;

			case CAIRO_PATH_LINE_TO:
				l = pathdist[i];

#define NEMOPATH_SPLIT_X(s, d, i, c)		((d.point.x - s.point.x) * (double)i / (double)c + s.point.x)
#define NEMOPATH_SPLIT_Y(s, d, i, c)		((d.point.y - s.point.y) * (double)i / (double)c + s.point.y)

				if (remain >= elements) {
					nemopath_line_to(path,
							data[1].point.x,
							data[1].point.y);

					elements -= 1;
				} else {
					if (l > length / elements) {
						if (remain > 1) {
							count = floor(l / (length / elements) + 0.5f);
						} else {
							count = elements;
						}

						for (j = 1; j <= count; j++) {
							nemopath_line_to(path,
									NEMOPATH_SPLIT_X(cp, data[1], j, count),
									NEMOPATH_SPLIT_Y(cp, data[1], j, count));

							elements -= 1;
						}
					}
				}

				cp = data[1];
				break;

			default:
				break;
		}

		length -= l;
		remain -= 1;
	}

	free(pathdata);
	free(pathdist);

	return 0;
}

int nemopath_tween(struct nemopath *path, struct nemopath *spath, struct nemopath *dpath, double progress)
{
	cairo_path_data_t *data, *sdata, *ddata;
	cairo_path_data_t cp, lp;
	int i;

	if (path->npathdata != spath->npathdata || path->npathdata != dpath->npathdata)
		return -1;

	path->extents[0] = FLT_MAX;
	path->extents[1] = FLT_MAX;
	path->extents[2] = -FLT_MAX;
	path->extents[3] = -FLT_MAX;

	path->length = 0.0f;
	path->npathdist = 0;

#define NEMOPATH_TWEEN_X(s, d, i, p)		((d[i].point.x - s[i].point.x) * p + s[i].point.x)
#define NEMOPATH_TWEEN_Y(s, d, i, p)		((d[i].point.y - s[i].point.y) * p + s[i].point.y)

	for (i = 0; i < path->npathdata; i += path->pathdata[i].header.length) {
		data = &path->pathdata[i];
		sdata = &spath->pathdata[i];
		ddata = &dpath->pathdata[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
				data[1].point.x = NEMOPATH_TWEEN_X(sdata, ddata, 1, progress);
				data[1].point.y = NEMOPATH_TWEEN_Y(sdata, ddata, 1, progress);

				lp = data[1];
				cp = data[1];
				break;

			case CAIRO_PATH_CLOSE_PATH:
				data = (&lp) - 1;

				CAIRO_POINT_MINMAX(cp, path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
				CAIRO_POINT_MINMAX(data[1], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);

				cp = data[1];
				break;

			case CAIRO_PATH_LINE_TO:
				data[1].point.x = NEMOPATH_TWEEN_X(sdata, ddata, 1, progress);
				data[1].point.y = NEMOPATH_TWEEN_Y(sdata, ddata, 1, progress);

				CAIRO_POINT_MINMAX(cp, path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
				CAIRO_POINT_MINMAX(data[1], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);

				cp = data[1];
				break;

			case CAIRO_PATH_CURVE_TO:
				data[1].point.x = NEMOPATH_TWEEN_X(sdata, ddata, 1, progress);
				data[1].point.y = NEMOPATH_TWEEN_Y(sdata, ddata, 1, progress);
				data[2].point.x = NEMOPATH_TWEEN_X(sdata, ddata, 2, progress);
				data[2].point.y = NEMOPATH_TWEEN_Y(sdata, ddata, 2, progress);
				data[3].point.x = NEMOPATH_TWEEN_X(sdata, ddata, 3, progress);
				data[3].point.y = NEMOPATH_TWEEN_Y(sdata, ddata, 3, progress);

				CAIRO_POINT_MINMAX(cp, path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
				CAIRO_POINT_MINMAX(data[1], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
				CAIRO_POINT_MINMAX(data[2], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);
				CAIRO_POINT_MINMAX(data[3], path->extents[0], path->extents[1], path->extents[2], path->extents[3]);

				cp = data[3];
				break;

			default:
				break;
		}
	}

	return 0;
}

void nemopath_extents(struct nemopath *path, double *extents)
{
	extents[0] = path->extents[0];
	extents[1] = path->extents[1];
	extents[2] = path->extents[2];
	extents[3] = path->extents[3];
}

int nemopath_elements(struct nemopath *path)
{
	cairo_path_data_t *data;
	int elements = 0;
	int i;

	for (i = 0; i < path->npathdata; i += path->pathdata[i].header.length) {
		data = &path->pathdata[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
			case CAIRO_PATH_CLOSE_PATH:
			case CAIRO_PATH_LINE_TO:
			case CAIRO_PATH_CURVE_TO:
				elements++;
				break;

			default:
				break;
		}
	}

	return elements;
}

double nemopath_width(struct nemopath *path)
{
	return path->extents[2];
}

double nemopath_height(struct nemopath *path)
{
	return path->extents[3];
}

double nemopath_length(struct nemopath *path)
{
	if (path->npathdist != path->npathdata)
		nemopath_update_length(path);

	return path->length;
}

void nemopath_reset(struct nemopath *path)
{
	path->index = 0;
	path->offset = 0.0f;
}

void nemopath_dump(struct nemopath *path, FILE *out)
{
	cairo_path_data_t *data;
	int i;

	for (i = 0; i < path->npathdata; i += path->pathdata[i].header.length) {
		data = &path->pathdata[i];

		switch (data->header.type) {
			case CAIRO_PATH_MOVE_TO:
				fprintf(out, "[MOVE] (%f, %f)\n",
						data[1].point.x, data[1].point.y);
				break;

			case CAIRO_PATH_CLOSE_PATH:
				fprintf(out, "[CLOSE]\n");
				break;

			case CAIRO_PATH_LINE_TO:
				fprintf(out, "[LINE] (%f, %f)\n",
						data[1].point.x, data[1].point.y);
				break;

			case CAIRO_PATH_CURVE_TO:
				fprintf(out, "[CURVE] (%f, %f) (%f, %f) (%f, %f)\n",
						data[1].point.x, data[1].point.y,
						data[2].point.x, data[2].point.y,
						data[3].point.x, data[3].point.y);
				break;

			default:
				break;
		}
	}
}
