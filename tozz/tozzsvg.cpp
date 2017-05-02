#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <tozzsvg.hpp>
#include <nemoxml.h>
#include <nemotoken.h>
#include <nemostring.h>
#include <nemomisc.h>

static int nemotozz_svg_get_transform_args(struct nemotoken *token, int offset, double *args)
{
	const char *value;
	int i;

	for (i = 0; ; i++) {
		value = nemotoken_get_token(token, i + offset);
		if (value == NULL || nemostring_is_number(value, 0, strlen(value)) == 0)
			break;

		args[i] = strtod(value, NULL);
	}

	return i;
}

static int nemotozz_svg_get_transform(SkMatrix *matrix, const char *value)
{
	struct nemotoken *token;
	const char *type;
	double args[8];
	int nargs;
	int i, count;

	token = nemotoken_create(value, strlen(value));
	if (token == NULL)
		return -1;
	nemotoken_divide(token, ' ');
	nemotoken_divide(token, '(');
	nemotoken_divide(token, ')');
	nemotoken_divide(token, ',');
	nemotoken_divide(token, '\t');
	nemotoken_update(token);

	count = nemotoken_get_count(token);
	i = 0;

	while (i < count) {
		type = nemotoken_get_token(token, i++);
		if (type != NULL) {
			if (strcmp(type, "translate") == 0) {
				nargs = nemotozz_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->postTranslate(args[0], 0.0f);
				} else if (nargs == 2) {
					matrix->postTranslate(args[0], args[1]);
				}

				i += nargs;
			} else if (strcmp(type, "rotate") == 0) {
				nargs = nemotozz_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->postRotate(args[0]);
				} else if (nargs == 3) {
					matrix->postTranslate(args[1], args[2]);
					matrix->postRotate(args[0]);
					matrix->postTranslate(-args[1], -args[2]);
				}

				i += nargs;
			} else if (strcmp(type, "scale") == 0) {
				nargs = nemotozz_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->postScale(args[0], 0.0f);
				} else if (nargs == 2) {
					matrix->postScale(args[0], args[1]);
				}

				i += nargs;
			} else if (strcmp(type, "skewX") == 0) {
				nargs = nemotozz_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->postSkew(args[0], 0.0f);
				}

				i += nargs;
			} else if (strcmp(type, "skewY") == 0) {
				nargs = nemotozz_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->postSkew(0.0f, args[0]);
				}

				i += nargs;
			} else if (strcmp(type, "matrix") == 0) {
				nargs = nemotozz_svg_get_transform_args(token, i, args);
				if (nargs == 6) {
					SkScalar targs[9] = {
						SkDoubleToScalar(args[0]), SkDoubleToScalar(args[2]), SkDoubleToScalar(args[4]),
						SkDoubleToScalar(args[1]), SkDoubleToScalar(args[3]), SkDoubleToScalar(args[5]),
						0.0f, 0.0f, 1.0f
					};

					matrix->set9(targs);
				}

				i += nargs;
			}
		}
	}

	nemotoken_destroy(token);

	return 0;
}

int nemotozz_svg_load(const char *url, float x, float y, float w, float h, SkPath *path)
{
	struct nemoxml *xml;
	struct xmlnode *node;
	const char *attr0, *attr1;
	float width = w, height = h;
	SkPath spath;
	SkMatrix smatrix;

	xml = nemoxml_create();
	nemoxml_load_file(xml, url);
	nemoxml_update(xml);

	nemolist_for_each(node, &xml->nodes, nodelink) {
		if ((attr0 = nemoxml_node_get_attr(node, "display")) != NULL && strcmp(attr0, "none") == 0)
			continue;

		spath.reset();

		if (strcmp(node->name, "path") == 0) {
			const char *d;

			d = nemoxml_node_get_attr(node, "d");
			if (d != NULL)
				SkParsePath::FromSVGString(d, &spath);
		} else if (strcmp(node->name, "line") == 0) {
			double x1 = nemoxml_node_get_dattr(node, "x1", 0.0f);
			double y1 = nemoxml_node_get_dattr(node, "y1", 0.0f);
			double x2 = nemoxml_node_get_dattr(node, "x2", 0.0f);
			double y2 = nemoxml_node_get_dattr(node, "y2", 0.0f);

			spath.moveTo(x1, y1);
			spath.lineTo(x2, y2);
		} else if (strcmp(node->name, "rect") == 0) {
			double x = nemoxml_node_get_dattr(node, "x", 0.0f);
			double y = nemoxml_node_get_dattr(node, "y", 0.0f);
			double width = nemoxml_node_get_dattr(node, "width", 0.0f);
			double height = nemoxml_node_get_dattr(node, "height", 0.0f);

			spath.addRect(x, y, x + width, y + height);
		} else if (strcmp(node->name, "circle") == 0) {
			double x = nemoxml_node_get_dattr(node, "cx", 0.0f);
			double y = nemoxml_node_get_dattr(node, "cy", 0.0f);
			double r = nemoxml_node_get_dattr(node, "r", 0.0f);

			spath.addCircle(x, y, r);
		} else if (strcmp(node->name, "ellipse") == 0) {
			double x = nemoxml_node_get_dattr(node, "cx", 0.0f);
			double y = nemoxml_node_get_dattr(node, "cy", 0.0f);
			double rx = nemoxml_node_get_dattr(node, "rx", 0.0f);
			double ry = nemoxml_node_get_dattr(node, "ry", 0.0f);
			SkRect oval = SkRect::MakeXYWH(x - rx, y - ry, rx * 2, ry * 2);

			spath.addArc(oval, 0, 360);
		} else if (strcmp(node->name, "polygon") == 0) {
			const char *points = nemoxml_node_get_attr(node, "points");
			struct nemotoken *token;
			int i, count;

			token = nemotoken_create(points, strlen(points));
			nemotoken_divide(token, ' ');
			nemotoken_divide(token, '\t');
			nemotoken_divide(token, ',');
			nemotoken_update(token);

			count = nemotoken_get_count(token);

			spath.moveTo(
					nemotoken_get_double(token, 0, 0.0f),
					nemotoken_get_double(token, 1, 0.0f));

			for (i = 2; i < count; i += 2) {
				spath.lineTo(
						nemotoken_get_double(token, i + 0, 0.0f),
						nemotoken_get_double(token, i + 1, 0.0f));
			}

			spath.close();

			nemotoken_destroy(token);
		} else if (strcmp(node->name, "polyline") == 0) {
			const char *points = nemoxml_node_get_attr(node, "points");
			struct nemotoken *token;
			int i, count;

			token = nemotoken_create(points, strlen(points));
			nemotoken_divide(token, ' ');
			nemotoken_divide(token, '\t');
			nemotoken_divide(token, ',');
			nemotoken_update(token);

			count = nemotoken_get_count(token);

			spath.moveTo(
					nemotoken_get_double(token, 0, 0.0f),
					nemotoken_get_double(token, 1, 0.0f));

			for (i = 2; i < count; i += 2) {
				spath.lineTo(
						nemotoken_get_double(token, i + 0, 0.0f),
						nemotoken_get_double(token, i + 1, 0.0f));
			}

			nemotoken_destroy(token);
		} else if (strcmp(node->name, "svg") == 0) {
			attr0 = nemoxml_node_get_attr(node, "width");
			attr1 = nemoxml_node_get_attr(node, "height");

			if (attr0 != NULL && attr1 != NULL) {
				width = nemostring_parse_float(attr0, 0, strlen(attr0));
				height = nemostring_parse_float(attr1, 0, strlen(attr1));
			}
		}

		if ((attr0 = nemoxml_node_get_attr(node, "transform")) != NULL) {
			nemotozz_svg_get_transform(&smatrix, attr0);

			spath.transform(smatrix);
		}

		path->addPath(spath);
	}

	nemoxml_destroy(xml);

	smatrix.setIdentity();
	smatrix.postScale(w / width, h / height);
	smatrix.postTranslate(x, y);

	path->transform(smatrix);

	return 0;
}
