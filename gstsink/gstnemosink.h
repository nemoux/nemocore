#ifndef __GST_NEMO_SINK_H__
#define __GST_NEMO_SINK_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/gstvideometa.h>

#include <wayland-client.h>

#include <gstnemobufferpool.h>

G_BEGIN_DECLS

#define GST_TYPE_NEMO_SINK \
	(gst_nemo_sink_get_type())
#define GST_NEMO_SINK(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NEMO_SINK, GstNemoSink))
#define GST_NEMO_SINK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_NEMO_SINK, GstNemoSinkClass))
#define GST_IS_NEMO_SINK(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_NEMO_SINK))
#define GST_IS_NEMO_SINK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_NEMO_SINK))
#define GST_NEMO_SINK_GET_CLASS(inst) \
	(G_TYPE_INSTANCE_GET_CLASS ((inst), GST_TYPE_NEMO_SINK, GstNemoSinkClass))

typedef struct _GstNemoSink				GstNemoSink;
typedef struct _GstNemoSinkClass	GstNemoSinkClass;

struct _GstNemoSink {
	GstVideoSink parent;

	struct wl_display *display;
	struct wl_shm *shm;
	struct wl_surface *surface;
	struct wl_buffer *buffer;
	uint32_t shm_formats;
	uint32_t width, height;

	gboolean redraw_pending;

	GstBufferPool *pool;
	GstBuffer *last_buffer;

	GMutex nemo_lock;

	gint video_width;
	gint video_height;
	uint32_t formats;
};

struct _GstNemoSinkClass {
	GstVideoSinkClass parent;
};

GType gst_nemo_sink_get_type(void) G_GNUC_CONST;

gboolean gst_nemo_sink_plugin_init(GstPlugin *plugin);

G_END_DECLS

#endif
