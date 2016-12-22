#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemoegl.h>

#include <nemoplay.h>
#include <playback.h>
#include <nemocook.h>
#include <nemofs.h>
#include <nemomisc.h>

struct nemoart {
	struct nemotool *tool;
	struct nemocanvas *canvas;

	int width, height;

	struct fsdir *contents;
	int icontents;

	struct cookegl *egl;

	struct nemoplay *play;
	struct playdecoder *decoderback;
	struct playaudio *audioback;
	struct playvideo *videoback;
	struct playshader *shader;
};

static void nemoart_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemoart *art = (struct nemoart *)nemocanvas_get_userdata(canvas);

	art->width = width;
	art->height = height;

	nemocanvas_egl_resize(art->canvas, width, height);
	nemocanvas_opaque(art->canvas, 0, 0, width, height);
}

static void nemoart_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemoart *art = (struct nemoart *)nemocanvas_get_userdata(canvas);
}

static int nemoart_dispatch_canvas_event(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event)
{
	struct nemoart *art = (struct nemoart *)nemocanvas_get_userdata(canvas);

	return 0;
}

static void nemoart_dispatch_video_update(struct nemoplay *play, void *data)
{
	struct nemoart *art = (struct nemoart *)data;

	nemocook_egl_prerender(art->egl);

	nemoplay_shader_dispatch(art->shader);

	nemocook_egl_postrender(art->egl);

	nemocanvas_dispatch_frame(art->canvas);
}

static void nemoart_dispatch_video_done(struct nemoplay *play, void *data)
{
	struct nemoart *art = (struct nemoart *)data;

	nemoplay_video_destroy(art->videoback);
	nemoplay_audio_destroy(art->audioback);
	nemoplay_decoder_destroy(art->decoderback);

	nemoplay_destroy(art->play);

	art->icontents = (art->icontents + 1) % nemofs_dir_get_filecount(art->contents);

	art->play = nemoplay_create();
	nemoplay_load_media(art->play, nemofs_dir_get_filepath(art->contents, art->icontents));

	art->decoderback = nemoplay_decoder_create(art->play);
	art->audioback = nemoplay_audio_create_by_ao(art->play);
	art->videoback = nemoplay_video_create_by_timer(art->play, art->tool);
	nemoplay_video_set_texture(art->videoback, 0, art->width, art->height);
	nemoplay_video_set_update(art->videoback, nemoart_dispatch_video_update);
	nemoplay_video_set_done(art->videoback, nemoart_dispatch_video_done);
	nemoplay_video_set_data(art->videoback, art);
	art->shader = nemoplay_video_get_shader(art->videoback);
	nemoplay_shader_set_flip(art->shader, 1);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",				required_argument,		NULL,		'w' },
		{ "height",				required_argument,		NULL,		'h' },
		{ "fullscreen",		required_argument,		NULL,		'f' },
		{ "content",			required_argument,		NULL,		'c' },
		{ 0 }
	};

	struct nemoart *art;
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct cookegl *egl;
	char *fullscreenid = NULL;
	char *contentpath = NULL;
	int width = 1920;
	int height = 1080;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:f:c:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'f':
				fullscreenid = strdup(optarg);
				break;

			case 'c':
				contentpath = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (contentpath == NULL)
		return 0;

	art = (struct nemoart *)malloc(sizeof(struct nemoart));
	if (art == NULL)
		return -1;
	memset(art, 0, sizeof(struct nemoart));

	art->width = width;
	art->height = height;

	if (os_check_path_is_directory(contentpath) != 0) {
		art->contents = nemofs_dir_create(contentpath, 128);
		nemofs_dir_scan_extension(art->contents, "mp4");
		nemofs_dir_scan_extension(art->contents, "avi");
		nemofs_dir_scan_extension(art->contents, "mov");
		nemofs_dir_scan_extension(art->contents, "ts");
	} else {
		art->contents = nemofs_dir_create(NULL, 32);
		nemofs_dir_insert_file(art->contents, contentpath);
	}

	art->tool = tool = nemotool_create();
	nemotool_connect_wayland(tool, NULL);
	nemotool_connect_egl(tool);

	art->canvas = canvas = nemocanvas_egl_create(tool, width, height);
	nemocanvas_opaque(canvas, 0, 0, width, height);
	nemocanvas_set_nemosurface(canvas, NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_dispatch_resize(canvas, nemoart_dispatch_canvas_resize);
	nemocanvas_set_dispatch_frame(canvas, nemoart_dispatch_canvas_frame);
	nemocanvas_set_dispatch_event(canvas, nemoart_dispatch_canvas_event);
	nemocanvas_set_userdata(canvas, art);

	if (fullscreenid != NULL)
		nemocanvas_set_fullscreen(canvas, fullscreenid);

	art->egl = egl = nemocook_egl_create(
			NTEGL_DISPLAY(tool),
			NTEGL_CONTEXT(tool),
			NTEGL_CONFIG(tool),
			NTEGL_WINDOW(canvas));
	nemocook_egl_resize(egl, width, height);

	art->play = nemoplay_create();
	nemoplay_load_media(art->play, nemofs_dir_get_filepath(art->contents, art->icontents));

	art->decoderback = nemoplay_decoder_create(art->play);
	art->audioback = nemoplay_audio_create_by_ao(art->play);
	art->videoback = nemoplay_video_create_by_timer(art->play, tool);
	nemoplay_video_set_texture(art->videoback, 0, width, height);
	nemoplay_video_set_update(art->videoback, nemoart_dispatch_video_update);
	nemoplay_video_set_done(art->videoback, nemoart_dispatch_video_done);
	nemoplay_video_set_data(art->videoback, art);
	art->shader = nemoplay_video_get_shader(art->videoback);
	nemoplay_shader_set_flip(art->shader, 1);

	nemotool_run(tool);

	nemoplay_video_destroy(art->videoback);
	nemoplay_audio_destroy(art->audioback);
	nemoplay_decoder_destroy(art->decoderback);

	nemoplay_destroy(art->play);

	nemocook_egl_destroy(egl);

	nemocanvas_egl_destroy(canvas);

	nemotool_destroy(tool);

	nemofs_dir_destroy(art->contents);

	free(art);

	return 0;
}
