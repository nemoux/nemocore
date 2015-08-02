#ifndef __NEMOSHOW_CAMERA_HPP__
#define __NEMOSHOW_CAMERA_HPP__

#include <skiaconfig.hpp>

typedef struct _showcamera {
	SkMatrix *matrix;
} showcamera_t;

#define NEMOSHOW_CAMERA_CC(base, name)				(((showcamera_t *)((base)->cc))->name)

#endif
