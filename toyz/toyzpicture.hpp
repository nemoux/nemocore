#ifndef __NEMOTOYZ_PICTURE_HPP__
#define __NEMOTOYZ_PICTURE_HPP__

#include <skiaconfig.hpp>

struct toyzpicture {
	SkPictureRecorder recorder;
	sk_sp<SkPicture> picture;
};

#endif
