#ifndef __NEMOTOZZ_PICTURE_HPP__
#define __NEMOTOZZ_PICTURE_HPP__

#include <skiaconfig.hpp>

struct tozzpicture {
	SkPictureRecorder recorder;
	sk_sp<SkPicture> picture;
};

#endif
