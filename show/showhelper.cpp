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

	if (a0.isZero())
		return a1.length() * 2.0f;
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
	double l0 = 2 * sqrt(c * scba) + tcb;
	double l1 = 2 * sqrt(c * a) + b;

	if (l0 == 0.0f || l1 == 0.0f)
		return SkPoint::Distance(pts[0], pts[1]) + SkPoint::Distance(pts[1], pts[2]);

	return m0 * (tcb * sqrt(scba) - b * sqrt(a)) + m1 * (log(l0) - log(l1));
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
	SkPoint tts[3];
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
				nemoshow_helper_convert_cubic_to_quad(pts, tts);
				length += nemoshow_helper_get_quad_length(tts);
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

void nemoshow_helper_draw_path(SkPath &dst, SkPath *src, SkPaint *paint, double length, double from, double to)
{
	SkPath::RawIter iter(*src);
	SkPoint lts;
	SkPoint pts[4];
	SkPoint tts[4];
	SkPoint nts[16];
	SkScalar t[4];
	SkPath::Verb verb;
	double start = length * from;
	double remain = length * to;
	double l;

	while (((verb = iter.next(pts)) != SkPath::kDone_Verb) && remain > 0.0f) {
		if (start > 0.0f) {
			switch (verb) {
				case SkPath::kMove_Verb:
					dst.moveTo(pts[0]);
					lts = pts[0];
					break;

				case SkPath::kLine_Verb:
					l = SkPoint::Distance(pts[0], pts[1]);
					if (l > start && l <= remain) {
						dst.moveTo(
								SkPoint::Make(
									(pts[1].x() - pts[0].x()) * (start / l) + pts[0].x(),
									(pts[1].y() - pts[0].y()) * (start / l) + pts[0].y()));
						dst.lineTo(pts[1]);
					} else if (l > start) {
						dst.moveTo(
								SkPoint::Make(
									(pts[1].x() - pts[0].x()) * (start / l) + pts[0].x(),
									(pts[1].y() - pts[0].y()) * (start / l) + pts[0].y()));
						dst.lineTo(
								SkPoint::Make(
									(pts[1].x() - pts[0].x()) * (remain / l) + pts[0].x(),
									(pts[1].y() - pts[0].y()) * (remain / l) + pts[0].y()));
					}

					start -= l;
					remain -= l;
					break;

				case SkPath::kQuad_Verb:
					l = nemoshow_helper_get_quad_length(pts);
					SkConvertQuadToCubic(pts, tts);
					if (l > start && l <= remain) {
						SkChopCubicAt(tts, nts, start / l);
						dst.moveTo(nts[3]);
						dst.cubicTo(nts[4], nts[5], nts[6]);
					} else if (l > start) {
						t[0] = start / l;
						t[1] = remain / l;

						SkChopCubicAt(tts, nts, t, 2);
						dst.moveTo(nts[3]);
						dst.cubicTo(nts[4], nts[5], nts[6]);
					}

					start -= l;
					remain -= l;
					break;

				case SkPath::kConic_Verb:
					break;

				case SkPath::kCubic_Verb:
					nemoshow_helper_convert_cubic_to_quad(pts, tts);
					l = nemoshow_helper_get_quad_length(tts);
					if (l > start && l <= remain) {
						SkChopCubicAt(pts, nts, start / l);
						dst.moveTo(nts[3]);
						dst.cubicTo(nts[4], nts[5], nts[6]);
					} else if (l > start) {
						t[0] = start / l;
						t[1] = remain / l;

						SkChopCubicAt(pts, nts, t, 2);
						dst.moveTo(nts[3]);
						dst.cubicTo(nts[4], nts[5], nts[6]);
					}

					start -= l;
					remain -= l;
					break;

				case SkPath::kClose_Verb:
					break;

				default:
					break;
			}
		} else {
			switch (verb) {
				case SkPath::kMove_Verb:
					dst.moveTo(pts[0]);
					lts = pts[0];
					break;

				case SkPath::kLine_Verb:
					l = SkPoint::Distance(pts[0], pts[1]);
					if (l <= remain) {
						dst.lineTo(pts[1]);
					} else {
						dst.lineTo(
								SkPoint::Make(
									(pts[1].x() - pts[0].x()) * (remain / l) + pts[0].x(),
									(pts[1].y() - pts[0].y()) * (remain / l) + pts[0].y()));
					}

					remain -= l;
					break;

				case SkPath::kQuad_Verb:
					l = nemoshow_helper_get_quad_length(pts);
					SkConvertQuadToCubic(pts, tts);
					if (l <= remain) {
						dst.cubicTo(tts[1], tts[2], tts[3]);
					} else {
						SkChopCubicAt(tts, nts, remain / l);
						dst.cubicTo(nts[1], nts[2], nts[3]);
					}

					remain -= l;
					break;

				case SkPath::kConic_Verb:
					break;

				case SkPath::kCubic_Verb:
					nemoshow_helper_convert_cubic_to_quad(pts, tts);
					l = nemoshow_helper_get_quad_length(tts);
					if (l <= remain) {
						dst.cubicTo(pts[1], pts[2], pts[3]);
					} else {
						SkChopCubicAt(pts, nts, remain / l);
						dst.cubicTo(nts[1], nts[2], nts[3]);
					}

					remain -= l;
					break;

				case SkPath::kClose_Verb:
					break;

				default:
					break;
			}
		}
	}
}

void nemoshow_helper_evaluate_path(SkPath *src, double length, double t, double *x, double *y, double *r)
{
	SkPath::RawIter iter(*src);
	SkPoint lts;
	SkPoint pts[4];
	SkPoint tts[4];
	SkPoint nts[16];
	SkPath::Verb verb;
	SkPoint cts;
	SkVector tan;
	double remain = length * t;
	double tx, ty, tr;
	double l;

	while (((verb = iter.next(pts)) != SkPath::kDone_Verb) && remain >= 0.0f) {
		switch (verb) {
			case SkPath::kMove_Verb:
				lts = pts[0];
				tx = pts[0].x();
				ty = pts[0].y();
				tr = 0.0f;
				break;

			case SkPath::kLine_Verb:
				l = SkPoint::Distance(pts[0], pts[1]);
				if (l > remain) {
					tx = (pts[1].x() - pts[0].x()) * (remain / l) + pts[0].x();
					ty = (pts[1].y() - pts[0].y()) * (remain / l) + pts[0].y();
					tr = 0.0f;
				} else {
					tx = pts[1].x();
					ty = pts[1].y();
					tr = 0.0f;
				}

				remain -= l;
				break;

			case SkPath::kQuad_Verb:
				l = nemoshow_helper_get_quad_length(pts);
				SkConvertQuadToCubic(pts, tts);
				if (l > remain) {
					SkEvalCubicAt(tts, remain / l, &cts, &tan, NULL);

					tx = cts.x();
					ty = cts.y();
					tr = tan.x() / tan.y();
				} else {
					tx = pts[2].x();
					ty = pts[2].y();
					tr = 0.0f;
				}

				remain -= l;
				break;

			case SkPath::kConic_Verb:
				break;

			case SkPath::kCubic_Verb:
				nemoshow_helper_convert_cubic_to_quad(pts, tts);
				l = nemoshow_helper_get_quad_length(tts);
				if (l > remain) {
					SkEvalCubicAt(pts, remain / l, &cts, &tan, NULL);

					tx = cts.x();
					ty = cts.y();
					tr = tan.x() / tan.y();
				} else {
					tx = pts[3].x();
					ty = pts[3].y();
					tr = 0.0f;
				}

				remain -= l;
				break;

			case SkPath::kClose_Verb:
				break;

			default:
				break;
		}
	}

	if (x != NULL)
		*x = tx;
	if (y != NULL)
		*y = ty;
	if (r != NULL)
		*r = tr;
}
