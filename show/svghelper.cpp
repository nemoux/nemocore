#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiaconfig.hpp>

#include <svghelper.hpp>
#include <nemoshow.h>
#include <showitem.h>
#include <showitem.hpp>
#include <showshader.h>
#include <showshader.hpp>
#include <stringhelper.h>
#include <nemotoken.h>
#include <nemomisc.h>

static int nemoshow_svg_get_transform_args(struct nemotoken *token, int offset, double *args)
{
	const char *value;
	int i;

	for (i = 0; ; i++) {
		value = nemotoken_get_token(token, i + offset);
		if (value == NULL || string_is_number(value, 0, strlen(value)) == 0)
			break;

		args[i] = strtod(value, NULL);
	}

	return i;
}

int nemoshow_svg_get_transform(SkMatrix *matrix, const char *value)
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

	count = nemotoken_get_token_count(token);
	i = 0;

	while (i < count) {
		type = nemotoken_get_token(token, i++);
		if (type != NULL) {
			if (strcmp(type, "translate") == 0) {
				nargs = nemoshow_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->postTranslate(args[0], 0.0f);
				} else if (nargs == 2) {
					matrix->postTranslate(args[0], args[1]);
				}

				i += nargs;
			} else if (strcmp(type, "rotate") == 0) {
				nargs = nemoshow_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->postRotate(args[0]);
				} else if (nargs == 3) {
					matrix->postTranslate(args[1], args[2]);
					matrix->postRotate(args[0]);
					matrix->postTranslate(-args[1], -args[2]);
				}

				i += nargs;
			} else if (strcmp(type, "scale") == 0) {
				nargs = nemoshow_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->postScale(args[0], 0.0f);
				} else if (nargs == 2) {
					matrix->postScale(args[0], args[1]);
				}

				i += nargs;
			} else if (strcmp(type, "skewX") == 0) {
				nargs = nemoshow_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->postSkew(args[0], 0.0f);
				}

				i += nargs;
			} else if (strcmp(type, "skewY") == 0) {
				nargs = nemoshow_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->postSkew(0.0f, args[0]);
				}

				i += nargs;
			} else if (strcmp(type, "matrix") == 0) {
				nargs = nemoshow_svg_get_transform_args(token, i, args);
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
