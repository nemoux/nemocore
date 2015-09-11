#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <gsthelper.h>
#include <nemomisc.h>

struct nemogst *nemogst_create(void)
{
	struct nemogst *gst;

	gst = malloc(sizeof(struct nemogst));
	if (gst == NULL)
		return NULL;
	memset(gst, 0, sizeof(struct nemogst));

	return gst;
}

void nemogst_destroy(struct nemogst *gst)
{
	gst_element_set_state(gst->player, GST_STATE_NULL);

	if (gst->player != NULL)
		gst_object_unref(GST_OBJECT(gst->player));

	if (gst->busid > 0)
		g_source_remove(gst->busid);

	if (gst->uri != NULL)
		free(gst->uri);

	free(gst);
}

static gboolean nemogst_watch_bus(GstBus *bus, GstMessage *msg, gpointer data)
{
	struct nemogst *gst = (struct nemogst *)data;

	switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_TAG:
		case GST_MESSAGE_STEP_DONE:
		case GST_MESSAGE_STREAM_STATUS:
		case GST_MESSAGE_DURATION:
		case GST_MESSAGE_ELEMENT:
			break;

		case GST_MESSAGE_BUFFERING:
			break;

		case GST_MESSAGE_REQUEST_STATE:
			break;

		case GST_MESSAGE_STATE_CHANGED:
			{
				GstState ostate, nstate, pstate;

				gst_message_parse_state_changed(msg, &ostate, &nstate, &pstate);

				if (GST_MESSAGE_SRC(msg) == GST_OBJECT(gst->player)) {
					gst->is_playing = (nstate == GST_STATE_PLAYING);

					if (gst->is_playing != 0) {
						GstQuery *query;

						query = gst_query_new_seeking(GST_FORMAT_TIME);
						if (gst_element_query(gst->player, query)) {
							gst_query_parse_seeking(query, NULL, &gst->is_seekable, &gst->seekstart, &gst->seekend);
						}

						gst_query_unref(query);
					}
				}
			}
			break;

		case GST_MESSAGE_SEGMENT_DONE:
			break;

		case GST_MESSAGE_EOS:
			break;

		case GST_MESSAGE_WARNING:
			{
				GError *err;
				gchar *debug;
				gchar *name;

				name = gst_object_get_path_string(GST_MESSAGE_SRC(msg));

				gst_message_parse_warning(msg, &err, &debug);

				g_error_free(err);
				g_free(debug);
				g_free(name);
			}
			break;

		case GST_MESSAGE_ERROR:
			{
				GError *err;
				gchar *debug;
				gchar *name;

				name = gst_object_get_path_string(GST_MESSAGE_SRC(msg));

				gst_message_parse_error(msg, &err, &debug);

				g_error_free(err);
				g_free(debug);
				g_free(name);
			}
			break;

		case GST_MESSAGE_ASYNC_DONE:
			gst->is_blocked = 0;
			break;

		default:
			break;
	}

	return GST_BUS_PASS;
}

int nemogst_prepare_nemo_sink(struct nemogst *gst, struct wl_display *display, struct wl_shm *shm, uint32_t formats, struct wl_surface *surface)
{
	GstPad *pad;

	gst->sink = gst_element_factory_make("nemosink", "nemosink");
	if (gst->sink == NULL)
		return -1;
	g_object_set(G_OBJECT(gst->sink), "wayland-display", display, NULL);
	g_object_set(G_OBJECT(gst->sink), "wayland-shm", shm, NULL);
	g_object_set(G_OBJECT(gst->sink), "wayland-formats", formats, NULL);
	g_object_set(G_OBJECT(gst->sink), "wayland-surface", surface, NULL);

	gst->player = gst_element_factory_make("playbin", "playbin");
	g_object_set(G_OBJECT(gst->player), "video-sink", gst->sink, NULL);

	gst->pipeline = gst_pipeline_new("nemopipe");
	gst->scale = gst_element_factory_make("videoscale", "nemoscale");
	gst->filter = gst_element_factory_make("capsfilter", "nemofilter");

	gst_bin_add_many(GST_BIN(gst->pipeline), gst->scale, gst->filter, gst->sink, NULL);
	gst_element_link_many(gst->scale, gst->filter, gst->sink, NULL);

	pad = gst_element_get_static_pad(gst->scale, "sink");
	gst_element_add_pad(gst->pipeline, gst_ghost_pad_new("sink", pad));
	gst_object_unref(pad);

	g_object_set(G_OBJECT(gst->player), "video-sink", gst->pipeline, NULL);

	gst->bus = gst_pipeline_get_bus(GST_PIPELINE(gst->player));
	gst->busid = gst_bus_add_watch(gst->bus, nemogst_watch_bus, gst);

	gst_element_set_state(gst->player, GST_STATE_READY);

	return 0;
}

int nemogst_prepare_nemo_subsink(struct nemogst *gst, nemogst_subtitle_render_t render, void *data)
{
	gst->subpipeline = gst_pipeline_new("nemosubpipe");

	gst->subfile = gst_element_factory_make("filesrc", "subfile");
	if (gst->subfile == NULL)
		return -1;
	gst->subparse = gst_element_factory_make("subparse", "subparse");
	if (gst->subparse == NULL)
		return -1;
	gst->subsink = gst_element_factory_make("nemosubsink", "subsink");
	if (gst->subsink == NULL)
		return -1;
	g_object_set(G_OBJECT(gst->subsink), "render-callback", render, NULL);
	g_object_set(G_OBJECT(gst->subsink), "render-data", data, NULL);

	gst_bin_add_many(GST_BIN(gst->subpipeline), gst->subfile, gst->subparse, gst->subsink, NULL);
	gst_element_link_many(gst->subfile, gst->subparse, gst->subsink, NULL);

	gst_element_set_state(gst->subpipeline, GST_STATE_READY);

	return 0;
}

int nemogst_prepare_mini_sink(struct nemogst *gst, nemogst_minisink_render_t render, void *data)
{
	GstPad *pad;

	gst->sink = gst_element_factory_make("minisink", "minisink");
	if (gst->sink == NULL)
		return -1;
	g_object_set(G_OBJECT(gst->sink), "render-callback", render, NULL);
	g_object_set(G_OBJECT(gst->sink), "render-data", data, NULL);

	gst->player = gst_element_factory_make("playbin", "playbin");
	g_object_set(G_OBJECT(gst->player), "video-sink", gst->sink, NULL);

	gst->pipeline = gst_pipeline_new("nemopipe");
	gst->scale = gst_element_factory_make("videoscale", "nemoscale");
	gst->filter = gst_element_factory_make("capsfilter", "nemofilter");

	gst_bin_add_many(GST_BIN(gst->pipeline), gst->scale, gst->filter, gst->sink, NULL);
	gst_element_link_many(gst->scale, gst->filter, gst->sink, NULL);

	pad = gst_element_get_static_pad(gst->scale, "sink");
	gst_element_add_pad(gst->pipeline, gst_ghost_pad_new("sink", pad));
	gst_object_unref(pad);

	g_object_set(G_OBJECT(gst->player), "video-sink", gst->pipeline, NULL);

	gst->bus = gst_pipeline_get_bus(GST_PIPELINE(gst->player));
	gst->busid = gst_bus_add_watch(gst->bus, nemogst_watch_bus, gst);

	gst_element_set_state(gst->player, GST_STATE_READY);

	return 0;
}

int nemogst_set_video_path(struct nemogst *gst, const char *uri)
{
	GstDiscoverer *dc;
	GstDiscovererInfo *info;
	GstDiscovererStreamInfo *sinfo;
	GstDiscovererVideoInfo *vinfo;
	GError *error = NULL;
	GList *list;
	gboolean has_video, has_audio;
	uint64_t duration;
	GstPlayFlags flags;

	dc = gst_discoverer_new(10 * GST_SECOND, &error);
	if (G_UNLIKELY(error))
		return -1;

	info = gst_discoverer_discover_uri(dc, uri, &error);
	if (G_UNLIKELY(error))
		return -1;

	list = gst_discoverer_info_get_video_streams(info);
	has_video = g_list_length(list) > 0;
	gst_discoverer_stream_info_list_free(list);

	list = gst_discoverer_info_get_audio_streams(info);
	has_audio = g_list_length(list) > 0;
	gst_discoverer_stream_info_list_free(list);

	if (has_video || has_audio)
		duration = gst_discoverer_info_get_duration(info);

	if (has_video) {
		list = gst_discoverer_info_get_video_streams(info);
		vinfo = (GstDiscovererVideoInfo *)list->data;
		gst->width = gst_discoverer_video_info_get_width(vinfo);
		gst->height = gst_discoverer_video_info_get_height(vinfo);
	} else {
		gst->width = 0;
		gst->height = 0;
	}

	gst_discoverer_info_unref(info);

	gst->video.width = gst->width;
	gst->video.height = gst->height;
	gst->video.aspect_ratio = (double)gst->video.width / (double)gst->video.height;
	gst->uri = strdup(uri);

	g_object_set(G_OBJECT(gst->player), "uri", uri, NULL);

	return 0;
}

int nemogst_set_subtitle_path(struct nemogst *gst, const char *path)
{
	g_object_set(G_OBJECT(gst->subfile), "location", path, NULL);

	return 0;
}

int nemogst_ready_video(struct nemogst *gst)
{
	gst_element_set_state(gst->player, GST_STATE_READY);

	if (gst->subpipeline != NULL)
		gst_element_set_state(gst->subpipeline, GST_STATE_READY);

	return 0;
}

int nemogst_play_video(struct nemogst *gst)
{
	gst_element_set_state(gst->player, GST_STATE_PLAYING);

	if (gst->subpipeline != NULL)
		gst_element_set_state(gst->subpipeline, GST_STATE_PLAYING);

	return 0;
}

int nemogst_pause_video(struct nemogst *gst)
{
	gst_element_set_state(gst->player, GST_STATE_PAUSED);

	if (gst->subpipeline != NULL)
		gst_element_set_state(gst->subpipeline, GST_STATE_PAUSED);

	return 0;
}

int nemogst_resize_video(struct nemogst *gst, uint32_t width, uint32_t height)
{
	GstCaps *caps;
	GstStructure *gs;

	caps = gst_caps_new_empty();
	gs = gst_structure_new("video/x-raw",
			"width", G_TYPE_INT, width,
			"height", G_TYPE_INT, height, NULL);
	gst_caps_append_structure(caps, gs);
	g_object_set(G_OBJECT(gst->filter), "caps", caps, NULL);
	gst_caps_unref(caps);

	gst->width = width;
	gst->height = height;

	return 0;
}

int64_t nemogst_get_position(struct nemogst *gst)
{
	gint64 position;

	if (!gst_element_query_position(gst->player, GST_FORMAT_TIME, &position))
		return 0;

	return position;
}

int64_t nemogst_get_duration(struct nemogst *gst)
{
	gint64 duration;

	if (!gst_element_query_duration(gst->player, GST_FORMAT_TIME, &duration))
		return 0;

	return duration;
}

int nemogst_set_position(struct nemogst *gst, int64_t position)
{
	if (gst->is_seekable != 0 && gst->is_blocked == 0 &&
			gst->seekstart <= position && gst->seekend >= position) {
		gst_element_seek_simple(gst->player,
				GST_FORMAT_TIME,
				GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT | GST_SEEK_FLAG_ACCURATE,
				position);

		if (gst->subpipeline != NULL) {
			gst_element_seek_simple(gst->subpipeline,
					GST_FORMAT_TIME,
					GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT | GST_SEEK_FLAG_ACCURATE,
					position);
		}

		gst->is_blocked = 1;
	}

	return 0;
}

int nemogst_set_position_rough(struct nemogst *gst, int64_t position)
{
	if (gst->is_seekable != 0 && gst->is_blocked == 0 &&
			gst->seekstart <= position && gst->seekend >= position) {
		gst_element_seek_simple(gst->player,
				GST_FORMAT_TIME,
				GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT | GST_SEEK_FLAG_KEY_UNIT,
				position);

		if (gst->subpipeline != NULL) {
			gst_element_seek_simple(gst->subpipeline,
					GST_FORMAT_TIME,
					GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT | GST_SEEK_FLAG_KEY_UNIT,
					position);
		}

		gst->is_blocked = 1;
	}

	return 0;
}

int nemogst_set_next_step(struct nemogst *gst, int steps, double rate)
{
	gboolean r;

	r = gst_element_send_event(gst->player, gst_event_new_step(GST_FORMAT_BUFFERS, steps, rate, TRUE, FALSE));

	if (gst->subpipeline != NULL) {
		gst_element_send_event(gst->subpipeline, gst_event_new_step(GST_FORMAT_BUFFERS, steps, rate, TRUE, FALSE));
	}

	return r;
}

void nemogst_set_audio_volume(struct nemogst *gst, double volume)
{
	volume = MIN(volume, 1.0f);
	volume = MAX(volume, 0.0f);

	g_object_set(G_OBJECT(gst->player), "volume", &volume, NULL);
}

static inline GstState nemogst_get_element_state(GstElement *element)
{
	GstState state;

	gst_element_get_state(element, &state, NULL, GST_CLOCK_TIME_NONE);

	return state;
}

void nemogst_dump_state(struct nemogst *gst)
{
	int i;

	NEMO_DEBUG("PLAYER = %s\n", gst_element_state_get_name(nemogst_get_element_state(gst->player)));
	NEMO_DEBUG("PIPELINE = %s\n", gst_element_state_get_name(nemogst_get_element_state(gst->pipeline)));
	NEMO_DEBUG("SINK = %s\n", gst_element_state_get_name(nemogst_get_element_state(gst->sink)));
}
