#ifndef __NEMOMOTZ_PICTURE_HPP__
#define __NEMOMOTZ_PICTURE_HPP__

#include <skiaconfig.hpp>

struct toyzpicture {
	SkPictureRecorder recorder;
	sk_sp<SkPicture> picture;
};

#endif
