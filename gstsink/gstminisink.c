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

#include <gstminisink.h>

enum {
	SIGNAL_0,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_SURFACE_WIDTH,
	PROP_SURFACE_HEIGHT,
	PROP_RENDER_CALLBACK,
	PROP_RENDER_DATA
};

GST_DEBUG_CATEGORY(gstmini_debug);
#define GST_CAT_DEFAULT gstmini_debug

#if G_BYTE_ORDER == G_BIG_ENDIAN
#define CAPS "{xRGB, ARGB}"
#else
#define CAPS "{BGRx, BGRA}"
#endif

#define	GST_MINI_SINK_BUFFER_ID			(0x1)

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink",
		GST_PAD_SINK,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE(CAPS)));

#define gst_mini_sink_parent_class parent_class
G_DEFINE_TYPE(GstMiniSink, gst_mini_sink, GST_TYPE_VIDEO_SINK);

static void gst_mini_sink_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void gst_mini_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_mini_sink_finalize(GObject *object);
static GstCaps *gst_mini_sink_get_caps(GstBaseSink *base, GstCaps *filter);
static gboolean gst_mini_sink_set_caps(GstBaseSink *base, GstCaps *caps);
static gboolean gst_mini_sink_start(GstBaseSink *base);
static gboolean gst_mini_sink_preroll(GstBaseSink *base, GstBuffer *buffer);
static gboolean gst_mini_sink_propose_allocation(GstBaseSink *base, GstQuery *query);
static gboolean gst_mini_sink_render(GstBaseSink *base, GstBuffer *buffer);

static void gst_mini_sink_class_init(GstMiniSinkClass *klass)
{
	GObjectClass *gobject_class = (GObjectClass *)klass;
	GstElementClass *element_class = (GstElementClass *)klass;
	GstBaseSinkClass *gstbasesink_class = (GstBaseSinkClass *)klass;

	gobject_class->set_property = gst_mini_sink_set_property;
	gobject_class->get_property = gst_mini_sink_get_property;
	gobject_class->finalize = GST_DEBUG_FUNCPTR(gst_mini_sink_finalize);

	gst_element_class_add_pad_template(element_class,
			gst_static_pad_template_get(&sink_template));

	gst_element_class_set_static_metadata(element_class,
			"mini video sink", "Sink/Video",
			"Output to wayland surface",
			"inhyeok.kim <haruband@gmail.com>");

	gstbasesink_class->get_caps = GST_DEBUG_FUNCPTR(gst_mini_sink_get_caps);
	gstbasesink_class->set_caps = GST_DEBUG_FUNCPTR(gst_mini_sink_set_caps);
	gstbasesink_class->start = GST_DEBUG_FUNCPTR(gst_mini_sink_start);
	gstbasesink_class->preroll = GST_DEBUG_FUNCPTR(gst_mini_sink_preroll);
	gstbasesink_class->propose_allocation =
		GST_DEBUG_FUNCPTR(gst_mini_sink_propose_allocation);
	gstbasesink_class->render = GST_DEBUG_FUNCPTR(gst_mini_sink_render);

	g_object_class_install_property(gobject_class, PROP_SURFACE_WIDTH,
			g_param_spec_uint("wayland-width", "Surface Width",
				"Surface width ",
				0, G_MAXINT, 0,
				G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(gobject_class, PROP_SURFACE_HEIGHT,
			g_param_spec_uint("wayland-height", "Surface Height",
				"Surface height ",
				0, G_MAXINT, 0,
				G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(gobject_class, PROP_RENDER_CALLBACK,
			g_param_spec_pointer("render-callback", "Render Callback",
				"Minisink rendering callback created by the application ",
				G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(gobject_class, PROP_RENDER_DATA,
			g_param_spec_pointer("render-data", "Render Data",
				"Minisink rendering data created by the application ",
				G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void gst_mini_sink_init(GstMiniSink *sink)
{
	sink->last_buffer = NULL;

	sink->callback = NULL;
	sink->data = NULL;

	g_mutex_init(&sink->mini_lock);
}

static void gst_mini_sink_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GstMiniSink *sink = GST_MINI_SINK(object);

	switch (prop_id) {
		case PROP_SURFACE_WIDTH:
			g_value_set_uint(value, sink->width);
			break;

		case PROP_SURFACE_HEIGHT:
			g_value_set_uint(value, sink->height);
			break;

		case PROP_RENDER_CALLBACK:
			g_value_set_pointer(value, sink->callback);
			break;

		case PROP_RENDER_DATA:
			g_value_set_pointer(value, sink->data);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void gst_mini_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GstMiniSink *sink = GST_MINI_SINK(object);

	switch (prop_id) {
		case PROP_SURFACE_WIDTH:
			sink->width = g_value_get_uint(value);
			break;

		case PROP_SURFACE_HEIGHT:
			sink->height = g_value_get_uint(value);
			break;

		case PROP_RENDER_CALLBACK:
			sink->callback = g_value_get_pointer(value);
			break;

		case PROP_RENDER_DATA:
			sink->data = g_value_get_pointer(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void gst_mini_sink_finalize(GObject *object)
{
	GstMiniSink *sink = GST_MINI_SINK(object);

	GST_DEBUG_OBJECT(sink, "Finalizing the sink..");

	if (sink->last_buffer != NULL)
		gst_buffer_unref(sink->last_buffer);

	g_mutex_clear(&sink->mini_lock);

	G_OBJECT_CLASS(parent_class)->finalize(object);
}

static GstCaps *gst_mini_sink_get_caps(GstBaseSink *base, GstCaps *filter)
{
	GstMiniSink *sink;
	GstCaps *caps;

	sink = GST_MINI_SINK(base);

	caps = gst_pad_get_pad_template_caps(GST_VIDEO_SINK_PAD(sink));
	if (filter) {
		GstCaps *intersection;

		intersection = gst_caps_intersect_full(filter, caps, GST_CAPS_INTERSECT_FIRST);
		gst_caps_unref(caps);
		caps = intersection;
	}

	return caps;
}

static GstVideoFormat gst_mini_sink_get_video_format(GstCaps *caps)
{
	GstStructure *structure;
	const gchar *format;
	GstVideoFormat fmt;

	structure = gst_caps_get_structure(caps, 0);
	format = gst_structure_get_string(structure, "format");
	fmt = gst_video_format_from_string(format);

	return fmt;
}

static gboolean gst_mini_sink_set_caps(GstBaseSink *base, GstCaps *caps)
{
	GstMiniSink *sink = GST_MINI_SINK(base);
	GstBufferPool *newpool, *oldpool;
	GstVideoInfo info;
	GstStructure *structure;
	static GstAllocationParams params = { 0, 0, 0, 15, };
	guint size;

	sink = GST_MINI_SINK(base);

	GST_LOG_OBJECT(sink, "set caps %" GST_PTR_FORMAT, caps);

	if (!gst_video_info_from_caps(&info, caps))
		goto invalid_format;

	sink->video_width = info.width;
	sink->video_height = info.height;
	sink->video_format = gst_mini_sink_get_video_format(caps);
	size = info.size;

	newpool = gst_mini_buffer_pool_new(GST_MINI_SINK_BUFFER_ID, sink->video_width, sink->video_height);
	if (!newpool) {
		GST_DEBUG_OBJECT(sink, "Failed to create new pool");
		return FALSE;
	}

	structure = gst_buffer_pool_get_config(newpool);
	gst_buffer_pool_config_set_params(structure, caps, size, 2, 0);
	gst_buffer_pool_config_set_allocator(structure, NULL, &params);
	if (!gst_buffer_pool_set_config(newpool, structure))
		goto config_failed;

	oldpool = sink->pool;
	sink->pool = newpool;
	if (oldpool)
		gst_object_unref(oldpool);

	return TRUE;

invalid_format:
	GST_DEBUG_OBJECT(sink,
			"Could not locate image format from caps %" GST_PTR_FORMAT, caps);
	return FALSE;

config_failed:
	GST_DEBUG_OBJECT(base, "failed setting config");
	return FALSE;
}

static gboolean gst_mini_sink_start(GstBaseSink *base)
{
	GstMiniSink *sink = (GstMiniSink *)base;
	gboolean result = TRUE;

	GST_DEBUG_OBJECT(sink, "start");

	return result;
}

static gboolean gst_mini_sink_propose_allocation(GstBaseSink *base, GstQuery *query)
{
	GstMiniSink *sink = GST_MINI_SINK(base);
	GstBufferPool *pool;
	GstStructure *config;
	GstCaps *caps;
	guint size;
	gboolean need_pool;

	gst_query_parse_allocation(query, &caps, &need_pool);

	if (caps == NULL)
		goto no_caps;

	g_mutex_lock(&sink->mini_lock);
	if ((pool = sink->pool))
		gst_object_ref(pool);
	g_mutex_unlock(&sink->mini_lock);

	if (pool != NULL) {
		GstCaps *pcaps;

		config = gst_buffer_pool_get_config(pool);
		gst_buffer_pool_config_get_params(config, &pcaps, &size, NULL, NULL);

		if (!gst_caps_is_equal(caps, pcaps)) {
			gst_object_unref(pool);
			pool = NULL;
		}
		gst_structure_free(config);
	}

	if (pool == NULL && need_pool) {
		GstVideoInfo info;

		if (!gst_video_info_from_caps(&info, caps))
			goto invalid_caps;

		GST_DEBUG_OBJECT(sink, "create new pool");
		pool = gst_mini_buffer_pool_new(GST_MINI_SINK_BUFFER_ID, sink->video_width, sink->video_height);

		size = info.size;

		config = gst_buffer_pool_get_config(pool);
		gst_buffer_pool_config_set_params(config, caps, size, 2, 0);
		if (!gst_buffer_pool_set_config(pool, config))
			goto config_failed;
	}

	if (pool) {
		gst_query_add_allocation_pool(query, pool, size, 2, 0);
		gst_object_unref(pool);
	}

	return TRUE;

no_caps:
	GST_DEBUG_OBJECT(base, "no caps specified");
	return FALSE;

invalid_caps:
	GST_DEBUG_OBJECT(base, "invalid caps specified");
	return FALSE;

config_failed:
	GST_DEBUG_OBJECT(base, "failed setting config");
	gst_object_unref(pool);
	return FALSE;
}

static GstFlowReturn gst_mini_sink_preroll(GstBaseSink *base, GstBuffer *buffer)
{
	GST_DEBUG_OBJECT(base, "preroll buffer %p", buffer);

	return gst_mini_sink_render(base, buffer);
}

static GstFlowReturn gst_mini_sink_render(GstBaseSink *base, GstBuffer *buffer)
{
	GstMiniSink *sink = GST_MINI_SINK(base);
	GstBuffer *to_render;
	GstMiniMeta *meta;
	GstMapInfo src;
	GstFlowReturn result = GST_FLOW_OK;

	GST_LOG_OBJECT(sink, "render buffer %p", buffer);

	meta = gst_buffer_get_mini_meta(buffer);

	if (meta && meta->id == GST_MINI_SINK_BUFFER_ID) {
		GST_LOG_OBJECT(sink, "buffer %p from our pool, writing directly", buffer);
		to_render = buffer;
	} else {
		GST_LOG_OBJECT(sink, "buffer %p not from our pool, copying", buffer);

		if (!gst_buffer_pool_is_active(sink->pool) &&
				!gst_buffer_pool_set_active(sink->pool, TRUE))
			goto activate_failed;

		result = gst_buffer_pool_acquire_buffer(sink->pool, &to_render, NULL);
		if (result != GST_FLOW_OK)
			goto no_buffer;

		gst_buffer_map(buffer, &src, GST_MAP_READ);
		gst_buffer_fill(to_render, 0, src.data, src.size);
		gst_buffer_unmap(buffer, &src);

		meta = gst_buffer_get_mini_meta(to_render);
	}

	if (G_UNLIKELY(to_render == sink->last_buffer))
		goto done;

	gst_buffer_replace(&sink->last_buffer, to_render);
	
	if (sink->callback != NULL) {
		sink->callback(GST_ELEMENT(base), sink->video_format, meta->data, meta->size, sink->data);
	}

	if (buffer != to_render)
		gst_buffer_unref(to_render);

	return GST_FLOW_OK;

no_buffer:
	GST_WARNING_OBJECT(sink, "could not create image");
	return result;

activate_failed:
	GST_ERROR_OBJECT(sink, "failed to activate bufferpool.");
	result = GST_FLOW_ERROR;

done:
	return result;
}

gboolean gst_mini_sink_plugin_init(GstPlugin *plugin)
{
	return gst_element_register(plugin,
			"minisink",
			GST_RANK_MARGINAL,
			GST_TYPE_MINI_SINK);
}
