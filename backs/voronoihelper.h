#ifndef	__NEMOUX_VORONOI_HELPER_H__
#define	__NEMOUX_VORONOI_HELPER_H__

#include <cstdio>
#include <vector>

#include <boost/polygon/voronoi.hpp>

using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;
using boost::polygon::x;
using boost::polygon::y;
using boost::polygon::low;
using boost::polygon::high;

struct Point {
	int a;
	int b;
	Point(int x, int y) : a(x), b(y) {}
};

struct Segment {
	Point p0;
	Point p1;
	Segment(int x1, int y1, int x2, int y2) : p0(x1, y1), p1(x2, y2) {}
};

namespace boost {
	namespace polygon {
		template <>
		struct geometry_concept<Point> {
			typedef point_concept type;
		};

		template <>
		struct point_traits<Point> {
			typedef int coordinate_type;
			
			static inline coordinate_type get(
					const Point& point, orientation_2d orient) {
				return (orient == HORIZONTAL) ? point.a : point.b;
			}
		};

		template <>
		struct geometry_concept<Segment> {
			typedef segment_concept type;
		};

		template <>
		struct segment_traits<Segment> {
			typedef int coordinate_type;
			typedef Point point_type;
			
			static inline point_type get(const Segment& segment, direction_1d dir) {
				return dir.to_int() ? segment.p1 : segment.p0;
			}
		};
	}
}

#endif
