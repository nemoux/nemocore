#ifndef __GST_MINI_BUFFER_POOL_H__
#define __GST_MINI_BUFFER_POOL_H__

#include <stdint.h>

#include <gst/gst.h>
#include <gst/video/video.h>

G_BEGIN_DECLS

typedef struct _GstMiniMeta GstMiniMeta;

typedef struct _GstMiniBufferPool GstMiniBufferPool;
typedef struct _GstMiniBufferPoolClass GstMiniBufferPoolClass;

GType gst_mini_meta_api_get_type(void);
#define GST_MINI_META_API_TYPE (gst_mini_meta_api_get_type())
const GstMetaInfo *gst_mini_meta_get_info(void);
#define GST_MINI_META_INFO (gst_mini_meta_get_info())

#define gst_buffer_get_mini_meta(b)				((GstMiniMeta *)gst_buffer_get_meta((b), GST_MINI_META_API_TYPE))

struct _GstMiniMeta {
	GstMeta meta;

	guint id;

	void *data;
	size_t size;
};

#define GST_TYPE_MINI_BUFFER_POOL				(gst_mini_buffer_pool_get_type())
#define GST_IS_MINI_BUFFER_POOL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_MINI_BUFFER_POOL))
#define GST_MINI_BUFFER_POOL(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_MINI_BUFFER_POOL, GstMiniBufferPool))
#define GST_MINI_BUFFER_POOL_CAST(obj)	((GstMiniBufferPool *)(obj))

struct _GstMiniBufferPool {
	GstBufferPool bufferpool;

	guint id;

	GstCaps *caps;
	GstVideoInfo info;
	guint width;
	guint height;
};

struct _GstMiniBufferPoolClass {
	GstBufferPoolClass parent_class;
};

GType gst_mini_buffer_pool_get_type(void);

GstBufferPool *gst_mini_buffer_pool_new(guint id, gint width, gint height);

G_END_DECLS

#endif
