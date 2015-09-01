#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

#include <gst/gstinfo.h>
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

#include <gstminibufferpool.h>

GType gst_mini_meta_api_get_type(void)
{
	static volatile GType type;
	static const gchar *tags[] = { "memory", "size", "colorspace", "orientation", NULL };

	if (g_once_init_enter(&type)) {
		GType _type = gst_meta_api_type_register("GstMiniMetaAPI", tags);
		g_once_init_leave(&type, _type);
	}

	return type;
}

static void gst_mini_meta_free(GstMiniMeta *meta, GstBuffer *buffer)
{
	munmap(meta->data, meta->size);
}

const GstMetaInfo *gst_mini_meta_get_info(void)
{
	static const GstMetaInfo *mini_meta_info = NULL;

	if (g_once_init_enter(&mini_meta_info)) {
		const GstMetaInfo *meta =
			gst_meta_register(GST_MINI_META_API_TYPE, "GstMiniMeta",
					sizeof(GstMiniMeta), (GstMetaInitFunction)NULL,
					(GstMetaFreeFunction)gst_mini_meta_free,
					(GstMetaTransformFunction)NULL);
		g_once_init_leave(&mini_meta_info, meta);
	}

	return mini_meta_info;
}

static void gst_mini_buffer_pool_finalize(GObject *object);

#define gst_mini_buffer_pool_parent_class parent_class
G_DEFINE_TYPE(GstMiniBufferPool, gst_mini_buffer_pool, GST_TYPE_BUFFER_POOL);

static gboolean gst_mini_buffer_pool_set_config(GstBufferPool *pool, GstStructure *config)
{
	GstMiniBufferPool *wpool = GST_MINI_BUFFER_POOL_CAST(pool);
	GstVideoInfo info;
	GstCaps *caps;

	if (!gst_buffer_pool_config_get_params(config, &caps, NULL, NULL, NULL))
		goto wrong_config;

	if (caps == NULL)
		goto no_caps;

	if (!gst_video_info_from_caps(&info, caps))
		goto wrong_caps;

	GST_LOG_OBJECT(pool, "%dx%d, caps %" GST_PTR_FORMAT, info.width, info.height, caps);

	wpool->caps = gst_caps_ref(caps);
	wpool->info = info;
	wpool->width = info.width;
	wpool->height = info.height;

	return GST_BUFFER_POOL_CLASS(parent_class)->set_config(pool, config);

wrong_config:
	GST_WARNING_OBJECT(pool, "invalid config");
	return FALSE;

no_caps:
	GST_WARNING_OBJECT(pool, "no caps in config");
	return FALSE;

wrong_caps:
	GST_WARNING_OBJECT(pool,
			"failed getting geometry from caps %" GST_PTR_FORMAT, caps);
	return FALSE;
}

static GstMiniMeta *gst_buffer_add_mini_meta(GstBuffer *buffer, GstMiniBufferPool *wpool)
{
	GstMiniMeta *wmeta;
	guint size = 0;
	static int sequence = 0;
	void *data;

	size = wpool->width * 4 * wpool->height;

	data = malloc(size);
	if (data == NULL) {
		GST_ERROR("alloc failed:");
		return NULL;
	}

	wmeta = (GstMiniMeta *)gst_buffer_add_meta(buffer, GST_MINI_META_INFO, NULL);
	wmeta->data = data;
	wmeta->size = size;

	gst_buffer_append_memory(
			buffer,
			gst_memory_new_wrapped(
				GST_MEMORY_FLAG_NO_SHARE,
				data,
				size,
				0,
				size,
				NULL,
				NULL));

	return wmeta;
}

static GstFlowReturn gst_mini_buffer_pool_alloc(GstBufferPool *pool, GstBuffer **buffer, GstBufferPoolAcquireParams *params)
{
	GstMiniBufferPool *wpool = GST_MINI_BUFFER_POOL_CAST(pool);
	GstBuffer *w_buffer;
	GstMiniMeta *meta;

	w_buffer = gst_buffer_new();
	meta = gst_buffer_add_mini_meta(w_buffer, wpool);
	if (meta == NULL) {
		gst_buffer_unref(w_buffer);
		goto no_buffer;
	}
	meta->id = wpool->id;

	*buffer = w_buffer;

	return GST_FLOW_OK;

no_buffer:
	GST_WARNING_OBJECT(pool, "can't create buffer");
	return GST_FLOW_ERROR;
}

GstBufferPool *gst_mini_buffer_pool_new(guint id, gint width, gint height)
{
	GstMiniBufferPool *pool;

	pool = g_object_new(GST_TYPE_MINI_BUFFER_POOL, NULL);
	pool->id = id;
	pool->width = width;
	pool->height = height;

	return GST_BUFFER_POOL_CAST(pool);
}

static void gst_mini_buffer_pool_class_init(GstMiniBufferPoolClass *klass)
{
	GObjectClass *gobject_class = (GObjectClass *)klass;
	GstBufferPoolClass *gstbufferpool_class = (GstBufferPoolClass *)klass;

	gobject_class->finalize = gst_mini_buffer_pool_finalize;

	gstbufferpool_class->set_config = gst_mini_buffer_pool_set_config;
	gstbufferpool_class->alloc_buffer = gst_mini_buffer_pool_alloc;
}

static void gst_mini_buffer_pool_init(GstMiniBufferPool *pool)
{
}

static void gst_mini_buffer_pool_finalize(GObject *object)
{
	GstMiniBufferPool *pool = GST_MINI_BUFFER_POOL_CAST(object);

	G_OBJECT_CLASS(gst_mini_buffer_pool_parent_class)->finalize(object);
}
