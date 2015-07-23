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

#include <gstnemosubsink.h>

enum {
	SIGNAL_0,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_RENDER_CALLBACK,
	PROP_RENDER_DATA
};

#define	GST_NEMO_SUBSINK_BUFFER_ID			(0x2)

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink",
		GST_PAD_SINK,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS_ANY);

#define gst_nemo_subsink_parent_class parent_class
G_DEFINE_TYPE(GstNemoSubSink, gst_nemo_subsink, GST_TYPE_BASE_SINK);

static void gst_nemo_subsink_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GstNemoSubSink *subsink = GST_NEMO_SUBSINK(object);

	switch (prop_id) {
		case PROP_RENDER_CALLBACK:
			g_value_set_pointer(value, subsink->callback);
			break;

		case PROP_RENDER_DATA:
			g_value_set_pointer(value, subsink->data);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void gst_nemo_subsink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GstNemoSubSink *subsink = GST_NEMO_SUBSINK(object);

	switch (prop_id) {
		case PROP_RENDER_CALLBACK:
			subsink->callback = g_value_get_pointer(value);
			break;

		case PROP_RENDER_DATA:
			subsink->data = g_value_get_pointer(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static gboolean gst_nemo_subsink_unlock_start(GstBaseSink *base)
{
	GstNemoSubSink *subsink = GST_NEMO_SUBSINK(base);

	return TRUE;
}

static gboolean gst_nemo_subsink_unlock_stop(GstBaseSink *base)
{
	GstNemoSubSink *subsink = GST_NEMO_SUBSINK(base);

	return TRUE;
}

static gboolean gst_nemo_subsink_start(GstBaseSink *base)
{
	GstNemoSubSink *subsink = GST_NEMO_SUBSINK(base);

	return TRUE;
}

static gboolean gst_nemo_subsink_stop(GstBaseSink *base)
{
	GstNemoSubSink *subsink = GST_NEMO_SUBSINK(base);

	return TRUE;
}

static gboolean gst_nemo_subsink_event(GstBaseSink *base, GstEvent *event)
{
	GstNemoSubSink *subsink = GST_NEMO_SUBSINK(base);

	switch (event->type) {
		default:
			break;
	}

	return GST_BASE_SINK_CLASS(parent_class)->event(base, event);
}

static GstFlowReturn gst_nemo_subsink_preroll(GstBaseSink *base, GstBuffer *buffer)
{
	GstNemoSubSink *subsink = GST_NEMO_SUBSINK(base);

	return GST_FLOW_OK;
}

static GstFlowReturn gst_nemo_subsink_render(GstBaseSink *base, GstBuffer *buffer)
{
	GstNemoSubSink *subsink = GST_NEMO_SUBSINK(base);
	GstMapInfo src;

	gst_buffer_map(buffer, &src, GST_MAP_READ);

	if (subsink->callback != NULL)
		subsink->callback(GST_ELEMENT(base), src.data, src.size, subsink->data);

	gst_buffer_unmap(buffer, &src);

	return GST_FLOW_OK;
}

static gboolean gst_nemo_subsink_query(GstBaseSink *base, GstQuery *query)
{
	GstNemoSubSink *subsink = GST_NEMO_SUBSINK(base);
	gboolean result;

	switch (GST_QUERY_TYPE(query)) {
		default:
			result = GST_BASE_SINK_CLASS(parent_class)->query(base, query);
			break;
	}

	return result;
}

static void gst_nemo_subsink_init(GstNemoSubSink *subsink)
{
	subsink->callback = NULL;
	subsink->data = NULL;
}

static void gst_nemo_subsink_dispose(GObject *object)
{
	GstNemoSubSink *subsink = GST_NEMO_SUBSINK(object);

	G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_nemo_subsink_finalize(GObject *object)
{
	GstNemoSubSink *subsink = GST_NEMO_SUBSINK(object);

	G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_nemo_subsink_class_init(GstNemoSubSinkClass *klass)
{
	GObjectClass *gobject_class = (GObjectClass *)klass;
	GstElementClass *element_class = (GstElementClass *)klass;
	GstBaseSinkClass *basesink_class = (GstBaseSinkClass *)klass;

	gobject_class->set_property = gst_nemo_subsink_set_property;
	gobject_class->get_property = gst_nemo_subsink_get_property;

	gobject_class->dispose = gst_nemo_subsink_dispose;
	gobject_class->finalize = gst_nemo_subsink_finalize;

	gst_element_class_add_pad_template(element_class,
			gst_static_pad_template_get(&sink_template));

	gst_element_class_set_static_metadata(element_class,
			"nemo subsink", "Video/Overlay/Subtitle",
			"Subtitle to wayland surface",
			"inhyeok.kim <haruband@gmail.com>");

	g_object_class_install_property(gobject_class, PROP_RENDER_CALLBACK,
			g_param_spec_pointer("render-callback", "Render Callback",
				"Subtitle rendering callback created by the application ",
				G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(gobject_class, PROP_RENDER_DATA,
			g_param_spec_pointer("render-data", "Render Data",
				"Subtitle rendering data created by the application ",
				G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	basesink_class->unlock = gst_nemo_subsink_unlock_start;
	basesink_class->unlock_stop = gst_nemo_subsink_unlock_stop;
	basesink_class->start = gst_nemo_subsink_start;
	basesink_class->stop = gst_nemo_subsink_stop;
	basesink_class->event = gst_nemo_subsink_event;
	basesink_class->preroll = gst_nemo_subsink_preroll;
	basesink_class->render = gst_nemo_subsink_render;
	basesink_class->query = gst_nemo_subsink_query;
}

gboolean gst_nemo_subsink_plugin_init(GstPlugin *plugin)
{
	return gst_element_register(plugin,
			"nemosubsink",
			GST_RANK_MARGINAL,
			GST_TYPE_NEMO_SUBSINK);
}
