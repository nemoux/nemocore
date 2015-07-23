#ifndef __GST_NEMO_SUBSINK_H__
#define __GST_NEMO_SUBSINK_H__

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>

G_BEGIN_DECLS

#define GST_TYPE_NEMO_SUBSINK \
	(gst_nemo_subsink_get_type())
#define GST_NEMO_SUBSINK(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NEMO_SUBSINK, GstNemoSubSink))
#define GST_NEMO_SUBSINK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_NEMO_SUBSINK, GstNemoSubSinkClass))
#define GST_IS_NEMO_SUBSINK(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_NEMO_SUBSINK))
#define GST_IS_NEMO_SUBSINK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_NEMO_SUBSINK))
#define GST_NEMO_SUBSINK_GET_CLASS(inst) \
	(G_TYPE_INSTANCE_GET_CLASS ((inst), GST_TYPE_NEMO_SUBSINK, GstNemoSubSinkClass))

typedef struct _GstNemoSubSink				GstNemoSubSink;
typedef struct _GstNemoSubSinkClass		GstNemoSubSinkClass;

typedef void (*GstNemoSubSinkRenderCallback)(GstElement *base, guint8 *buffer, gsize size, gpointer data);

struct _GstNemoSubSink {
	GstBaseSink parent;

	GstNemoSubSinkRenderCallback callback;
	gpointer data;

	GstPad *sinkpad;
};

struct _GstNemoSubSinkClass {
	GstBaseSinkClass parent;
};

GType gst_nemo_subsink_get_type(void) G_GNUC_CONST;

gboolean gst_nemo_subsink_plugin_init(GstPlugin *plugin);

G_END_DECLS

#endif
