#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showhelper.hpp>
#include <nemomisc.h>

static inline double nemoshow_helper_get_quad_length(SkPoint *pts)
{
	SkPoint a0 = pts[1] - pts[0];
	SkPoint a1 = pts[0] - pts[1] * 2.0f + pts[2];

	if (a1.isZero())
		return a0.length() * 2.0f;

	double c = 4 * a1.dot(a1);
	double b = 8 * a0.dot(a1);
	double a = 4 * a0.dot(a0);
	double q = 4 * a * c - b * b;
	double tcb = 2 * c + b;
	double scba = c + b + a;
	double m0 = 0.25f / c;
	double m1 = q / (8 * pow(c, 1.5f));

	return
		m0 * (tcb * sqrt(scba) - b * sqrt(a)) +
		m1 * (log(2 * sqrt(c * scba) + tcb) - log(2 * sqrt(c * a) + b));
}

static inline void nemoshow_helper_convert_cubic_to_quad(SkPoint *src, SkPoint *dst)
{
	SkPoint mp = (src[2] * 3.0f - src[3] + src[1] * 3.0f - src[0]) * 0.25f;

	dst[0] = src[0];
	dst[1] = mp;
	dst[2] = src[3];
}

double nemoshow_helper_get_path_length(SkPath *path)
{
	SkPath::RawIter iter(*path);
	SkPoint lts;
	SkPoint pts[4];
	SkPoint qts[3];
	SkPath::Verb verb;
	double length = 0.0f;

	while ((verb = iter.next(pts)) != SkPath::kDone_Verb) {
		switch (verb) {
			case SkPath::kMove_Verb:
				lts = pts[0];
				break;

			case SkPath::kLine_Verb:
				length += SkPoint::Distance(pts[0], pts[1]);
				break;

			case SkPath::kQuad_Verb:
				length += nemoshow_helper_get_quad_length(pts);
				break;

			case SkPath::kConic_Verb:
				break;

			case SkPath::kCubic_Verb:
				nemoshow_helper_convert_cubic_to_quad(pts, qts);
				length += nemoshow_helper_get_quad_length(qts);
				break;

			case SkPath::kClose_Verb:
				length += SkPoint::Distance(lts, pts[0]);
				break;

			default:
				break;
		}
	}

	return length;
}
