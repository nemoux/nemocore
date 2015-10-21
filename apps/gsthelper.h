#ifndef __NEMOUX_GST_HELPER_H__
#define __NEMOUX_GST_HELPER_H__

#include <stdint.h>

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/navigation.h>
#include <gst/pbutils/pbutils.h>

#include <wayland-client.h>

typedef enum {
	GST_PLAY_FLAG_VIDEO = 0x00000001,
	GST_PLAY_FLAG_AUDIO = 0x00000002,
	GST_PLAY_FLAG_TEXT = 0x00000004,
	GST_PLAY_FLAG_VIS = 0x00000008,
	GST_PLAY_FLAG_SOFT_VOLUME = 0x00000010,
	GST_PLAY_FLAG_NATIVE_AUDIO = 0x00000020,
	GST_PLAY_FLAG_NATIVE_VIDEO = 0x00000040,
	GST_PLAY_FLAG_DOWNLOAD = 0x00000080,
	GST_PLAY_FLAG_BUFFERING = 0x000000100,
	GST_PLAY_FLAG_DEINTERLACE = 0x000000200,
	GST_PLAY_FLAG_SOFT_COLORBALANCE = 0x000000400
} GstPlayFlags;

typedef void (*nemogst_subtitle_render_t)(GstElement *base, guint8 *data, gsize size, gpointer userdata);
typedef void (*nemogst_minisink_render_t)(GstElement *base, guint8 *data, gint width, gint height, GstVideoFormat format, gpointer userdata);

struct nemogst {
	GstElement *player;
	GstElement *pipeline;
	GstElement *sink;
	GstElement *scale;
	GstElement *filter;

	GstElement *subpipeline;
	GstElement *subfile;
	GstElement *subparse;
	GstElement *subsink;

	GstBus *bus;
	guint busid;

	struct {
		uint32_t width, height;
		double aspect_ratio;
	} video;

	uint32_t width, height;

	char *uri;

	int is_blocked;
	int is_changed;

	gboolean is_seekable;
	gint64 seekstart, seekend;

	int64_t position;
	int64_t duration;
};

extern struct nemogst *nemogst_create(void);
extern void nemogst_destroy(struct nemogst *gst);

extern int nemogst_prepare_nemo_sink(struct nemogst *gst, struct wl_display *display, struct wl_shm *shm, uint32_t formats, struct wl_surface *surface);
extern int nemogst_prepare_nemo_subsink(struct nemogst *gst, nemogst_subtitle_render_t render, void *userdata);
extern int nemogst_prepare_mini_sink(struct nemogst *gst, nemogst_minisink_render_t render, void *userdata);
extern int nemogst_prepare_audio_sink(struct nemogst *gst);

extern int nemogst_load_media_info(struct nemogst *gst, const char *uri);

extern int nemogst_set_media_path(struct nemogst *gst, const char *uri);
extern int nemogst_set_subtitle_path(struct nemogst *gst, const char *path);

extern int nemogst_ready_media(struct nemogst *gst);
extern int nemogst_play_media(struct nemogst *gst);
extern int nemogst_pause_media(struct nemogst *gst);
extern int nemogst_replay_media(struct nemogst *gst);
extern int nemogst_is_done_media(struct nemogst *gst);
extern int nemogst_is_playing_media(struct nemogst *gst);

extern int nemogst_resize_video(struct nemogst *gst, uint32_t width, uint32_t height);

extern void nemogst_sink_set_property(struct nemogst *gst, const char *name, uint32_t value);

extern int64_t nemogst_get_position(struct nemogst *gst);
extern int64_t nemogst_get_duration(struct nemogst *gst);
extern int nemogst_set_position(struct nemogst *gst, int64_t position);
extern int nemogst_set_next_step(struct nemogst *gst, int steps, double rate);

static inline uint32_t nemogst_get_video_width(struct nemogst *gst)
{
	return gst->video.width;
}

static inline uint32_t nemogst_get_video_height(struct nemogst *gst)
{
	return gst->video.height;
}

static inline double nemogst_get_video_aspect_ratio(struct nemogst *gst)
{
	return gst->video.aspect_ratio;
}

static inline uint32_t nemogst_get_width(struct nemogst *gst)
{
	return gst->width;
}

static inline uint32_t nemogst_get_height(struct nemogst *gst)
{
	return gst->height;
}

static inline int nemogst_is_blocked(struct nemogst *gst)
{
	return gst->is_blocked;
}

static inline int nemogst_is_changed(struct nemogst *gst)
{
	return gst->is_changed;
}

#endif
