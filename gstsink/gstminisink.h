#ifndef __GST_MINI_SINK_H__
#define __GST_MINI_SINK_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/gstvideometa.h>

#include <gstminibufferpool.h>

G_BEGIN_DECLS

#define GST_TYPE_MINI_SINK \
	(gst_mini_sink_get_type())
#define GST_MINI_SINK(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_MINI_SINK, GstMiniSink))
#define GST_MINI_SINK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_MINI_SINK, GstMiniSinkClass))
#define GST_IS_MINI_SINK(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_MINI_SINK))
#define GST_IS_MINI_SINK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_MINI_SINK))
#define GST_MINI_SINK_GET_CLASS(inst) \
	(G_TYPE_INSTANCE_GET_CLASS ((inst), GST_TYPE_MINI_SINK, GstMiniSinkClass))

typedef struct _GstMiniSink				GstMiniSink;
typedef struct _GstMiniSinkClass	GstMiniSinkClass;

typedef void (*GstMiniSinkRenderCallback)(GstElement *base, guint8 *data, gint width, gint height, GstVideoFormat format, gpointer userdata);

struct _GstMiniSink {
	GstVideoSink parent;

	uint32_t width, height;

	GstBufferPool *pool;
	GstBuffer *last_buffer;

	GMutex mini_lock;

	GstVideoFormat video_format;
	gint video_width;
	gint video_height;

	GstMiniSinkRenderCallback callback;
	gpointer userdata;

	gboolean redraw_pending;
};

struct _GstMiniSinkClass {
	GstVideoSinkClass parent;
};

GType gst_mini_sink_get_type(void) G_GNUC_CONST;

gboolean gst_mini_sink_plugin_init(GstPlugin *plugin);

G_END_DECLS

#endif
