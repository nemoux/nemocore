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

#include <gstnemobufferpool.h>

GType gst_wl_meta_api_get_type(void)
{
	static volatile GType type;
	static const gchar *tags[] = { "memory", "size", "colorspace", "orientation", NULL };

	if (g_once_init_enter(&type)) {
		GType _type = gst_meta_api_type_register("GstWlMetaAPI", tags);
		g_once_init_leave(&type, _type);
	}

	return type;
}

static void gst_wl_meta_free(GstWlMeta *meta, GstBuffer *buffer)
{
	munmap(meta->data, meta->size);

	if (meta->wbuffer != NULL)
		wl_buffer_destroy(meta->wbuffer);
}

const GstMetaInfo *gst_wl_meta_get_info(void)
{
	static const GstMetaInfo *wl_meta_info = NULL;

	if (g_once_init_enter(&wl_meta_info)) {
		const GstMetaInfo *meta =
			gst_meta_register(GST_WL_META_API_TYPE, "GstWlMeta",
					sizeof(GstWlMeta), (GstMetaInitFunction)NULL,
					(GstMetaFreeFunction)gst_wl_meta_free,
					(GstMetaTransformFunction)NULL);
		g_once_init_leave(&wl_meta_info, meta);
	}

	return wl_meta_info;
}

static void gst_nemo_buffer_pool_finalize(GObject *object);

#define gst_nemo_buffer_pool_parent_class parent_class
G_DEFINE_TYPE(GstNemoBufferPool, gst_nemo_buffer_pool, GST_TYPE_BUFFER_POOL);

static gboolean gst_nemo_buffer_pool_set_config(GstBufferPool *pool, GstStructure *config)
{
	GstNemoBufferPool *wpool = GST_NEMO_BUFFER_POOL_CAST(pool);
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

static GstWlMeta *gst_buffer_add_nemo_meta(GstBuffer *buffer, GstNemoBufferPool *wpool)
{
	GstWlMeta *wmeta;
	guint size = 0;
	struct wl_shm_pool *pool;
	static int sequence = 0;
	char filename[1024];
	void *data;
	int fd;

	size = wpool->width * 4 * wpool->height;

	snprintf(filename, 256, "%s-%d-%s", "/tmp/nemo-shm", sequence++, "XXXXXX");

	fd = mkstemp(filename);
	if (fd < 0) {
		GST_ERROR("open %s failed:", filename);
		return NULL;
	}
	if (ftruncate(fd, size) < 0) {
		GST_ERROR("ftruncate failed:");
		close(fd);
		return NULL;
	}

	data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		GST_ERROR("mmap failed:");
		close(fd);
		return NULL;
	}

	pool = wl_shm_create_pool(wpool->shm, fd, size);
	if (pool == NULL) {
		GST_ERROR("shmpool failed:");
		close(fd);
		return NULL;
	}

	wmeta = (GstWlMeta *)gst_buffer_add_meta(buffer, GST_WL_META_INFO, NULL);
	wmeta->wbuffer = wl_shm_pool_create_buffer(pool, 0,
			wpool->width, wpool->height, wpool->width * 4, wpool->formats);

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

	wl_shm_pool_destroy(pool);
	close(fd);

	return wmeta;
}

static GstFlowReturn gst_nemo_buffer_pool_alloc(GstBufferPool *pool, GstBuffer **buffer, GstBufferPoolAcquireParams *params)
{
	GstNemoBufferPool *wpool = GST_NEMO_BUFFER_POOL_CAST(pool);
	GstBuffer *w_buffer;
	GstWlMeta *meta;

	w_buffer = gst_buffer_new();
	meta = gst_buffer_add_nemo_meta(w_buffer, wpool);
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

GstBufferPool *gst_nemo_buffer_pool_new(guint id, struct wl_shm *shm, uint32_t formats, gint width, gint height)
{
	GstNemoBufferPool *pool;

	pool = g_object_new(GST_TYPE_NEMO_BUFFER_POOL, NULL);
	pool->id = id;
	pool->shm = shm;
	pool->formats = formats;
	pool->width = width;
	pool->height = height;

	return GST_BUFFER_POOL_CAST(pool);
}

static void gst_nemo_buffer_pool_class_init(GstNemoBufferPoolClass *klass)
{
	GObjectClass *gobject_class = (GObjectClass *)klass;
	GstBufferPoolClass *gstbufferpool_class = (GstBufferPoolClass *)klass;

	gobject_class->finalize = gst_nemo_buffer_pool_finalize;

	gstbufferpool_class->set_config = gst_nemo_buffer_pool_set_config;
	gstbufferpool_class->alloc_buffer = gst_nemo_buffer_pool_alloc;
}

static void gst_nemo_buffer_pool_init(GstNemoBufferPool *pool)
{
}

static void gst_nemo_buffer_pool_finalize(GObject *object)
{
	GstNemoBufferPool *pool = GST_NEMO_BUFFER_POOL_CAST(object);

	G_OBJECT_CLASS(gst_nemo_buffer_pool_parent_class)->finalize(object);
}
