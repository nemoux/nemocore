#ifndef	__NEMOTOZZ_SKIA_CONFIG_H__
#define	__NEMOTOZZ_SKIA_CONFIG_H__

#define	SK_RELEASE			1
#define	SK_CPU_LENDIAN	1
#define SK_R32_SHIFT		16
#define SK_G32_SHIFT		8
#define SK_B32_SHIFT		0
#define SK_A32_SHIFT		24

#include <SkTypes.h>
#include <SkCanvas.h>
#include <SkGraphics.h>
#include <SkImageInfo.h>
#include <SkImageEncoder.h>
#include <SkBitmapDevice.h>
#include <SkBitmap.h>
#include <SkCodec.h>
#include <SkPngChunkReader.h>
#include <SkStream.h>
#include <SkString.h>
#include <SkMatrix.h>
#include <SkRegion.h>
#include <SkParsePath.h>
#include <SkTypeface.h>
#include <SkTextBox.h>

#include <SkBlurMaskFilter.h>
#include <SkEmbossMaskFilter.h>
#include <SkDropShadowImageFilter.h>
#include <SkLumaColorFilter.h>

#include <SkGradientShader.h>
#include <SkPerlinNoiseShader.h>

#include <SkPathEffect.h>
#include <SkDiscretePathEffect.h>
#include <SkDashPathEffect.h>
#include <SkPathMeasure.h>

#include <SkPictureRecorder.h>

#endif
