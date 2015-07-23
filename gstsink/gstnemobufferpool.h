#ifndef __GST_NEMO_BUFFER_POOL_H__
#define __GST_NEMO_BUFFER_POOL_H__

#include <gst/gst.h>
#include <gst/video/video.h>

#include <wayland-client.h>

G_BEGIN_DECLS

typedef struct _GstWlMeta GstWlMeta;

typedef struct _GstNemoBufferPool GstNemoBufferPool;
typedef struct _GstNemoBufferPoolClass GstNemoBufferPoolClass;

GType gst_wl_meta_api_get_type(void);
#define GST_WL_META_API_TYPE (gst_wl_meta_api_get_type())
const GstMetaInfo *gst_wl_meta_get_info(void);
#define GST_WL_META_INFO (gst_wl_meta_get_info())

#define gst_buffer_get_wl_meta(b)				((GstWlMeta *)gst_buffer_get_meta((b), GST_WL_META_API_TYPE))

struct _GstWlMeta {
	GstMeta meta;

	guint id;

	struct wl_buffer *wbuffer;
	void *data;
	size_t size;
};

#define GST_TYPE_NEMO_BUFFER_POOL				(gst_nemo_buffer_pool_get_type())
#define GST_IS_NEMO_BUFFER_POOL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_NEMO_BUFFER_POOL))
#define GST_NEMO_BUFFER_POOL(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NEMO_BUFFER_POOL, GstNemoBufferPool))
#define GST_NEMO_BUFFER_POOL_CAST(obj)	((GstNemoBufferPool *)(obj))

struct _GstNemoBufferPool {
	GstBufferPool bufferpool;

	guint id;

	struct wl_shm *shm;
	uint32_t formats;

	GstCaps *caps;
	GstVideoInfo info;
	guint width;
	guint height;
};

struct _GstNemoBufferPoolClass {
	GstBufferPoolClass parent_class;
};

GType gst_nemo_buffer_pool_get_type(void);

GstBufferPool *gst_nemo_buffer_pool_new(guint id, struct wl_shm *shm, uint32_t formats, gint width, gint height);

G_END_DECLS

#endif
