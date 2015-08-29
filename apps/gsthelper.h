#ifndef	__NEMOTOOL_GST_HELPER_H__
#define	__NEMOTOOL_GST_HELPER_H__

#include <stdint.h>

#include <gst/gst.h>
#include <gst/video/navigation.h>
#include <gst/pbutils/pbutils.h>

#include <wayland-client.h>

#define	NEMOGST_AUDIO_SINK_MAX			(8)

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

typedef void (*nemogst_subtitle_render_t)(GstElement *base, guint8 *buffer, gsize size, gpointer data);

struct nemogst {
	GstElement *player;
	GstElement *pipeline;
	GstElement *sink;
	GstElement *scale;
	GstElement *filter;

#ifdef NEMOUX_WITH_ALSA_MULTINODE
	GstElement *audiobin;
	GstElement *audiotee;
	GstPadTemplate *audioteepadtempl;
	GstElement *audiosinks[NEMOGST_AUDIO_SINK_MAX];
	GstPad *audioteepads[NEMOGST_AUDIO_SINK_MAX];
	char *audiodevs[NEMOGST_AUDIO_SINK_MAX];
#endif

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

	int is_playing;
	int is_blocked;

	gboolean is_seekable;
	gint64 seekstart, seekend;
};

extern struct nemogst *nemogst_create(void);
extern void nemogst_destroy(struct nemogst *gst);

extern int nemogst_prepare_nemo_sink(struct nemogst *gst, struct wl_display *display, struct wl_shm *shm, uint32_t formats, struct wl_surface *surface);
extern int nemogst_prepare_nemo_subsink(struct nemogst *gst, nemogst_subtitle_render_t render, void *data);

extern int nemogst_set_video_path(struct nemogst *gst, const char *uri);
extern int nemogst_set_subtitle_path(struct nemogst *gst, const char *path);

extern int nemogst_ready_video(struct nemogst *gst);
extern int nemogst_play_video(struct nemogst *gst);
extern int nemogst_pause_video(struct nemogst *gst);
extern int nemogst_resize_video(struct nemogst *gst, uint32_t width, uint32_t height);

extern int64_t nemogst_get_position(struct nemogst *gst);
extern int64_t nemogst_get_duration(struct nemogst *gst);
extern int nemogst_set_position(struct nemogst *gst, int64_t position);
extern int nemogst_set_position_rough(struct nemogst *gst, int64_t position);
extern int nemogst_set_next_step(struct nemogst *gst, int steps, double rate);

extern void nemogst_set_audio_volume(struct nemogst *gst, double volume);

#ifdef NEMOUX_WITH_ALSA_MULTINODE
extern int nemogst_set_audio_device(struct nemogst *gst, const char *dev);
extern int nemogst_put_audio_device(struct nemogst *gst, const char *dev);
#endif

extern void nemogst_dump_state(struct nemogst *gst);

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

static inline int nemogst_is_playing(struct nemogst *gst)
{
	return gst->is_playing;
}

static inline int nemogst_is_blocked(struct nemogst *gst)
{
	return gst->is_blocked;
}

#endif
