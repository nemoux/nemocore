#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <pthread.h>

#include <ao/ao.h>

#include <nemoplay.h>
#include <nemomisc.h>

static void *nemoplay_handle_videoplay(void *arg)
{
	struct nemoplay *play = (struct nemoplay *)arg;
	struct playqueue *queue;
	struct playone *one;
	int iframe = 0;

	queue = nemoplay_get_video_queue(play);

	while ((one = nemoplay_queue_dequeue(queue)) != NULL) {
		FILE *file;
		char filename[32];
		int y;

		sprintf(filename, "frame%d.ppm", ++iframe);

		file = fopen(filename, "wb");
		if (file == NULL)
			break;

		fprintf(file, "P6\n%d %d\n255\n", one->width, one->height);

		for (y = 0; y < one->height; y++)
			fwrite(one->data + y * one->stride, 1, one->stride, file);

		fclose(file);

		free(one->data);

		nemoplay_queue_destroy_one(one);
	}

	return NULL;
}

static void *nemoplay_handle_audioplay(void *arg)
{
	struct nemoplay *play = (struct nemoplay *)arg;
	struct playqueue *queue;
	struct playone *one;
	ao_device *device;
	ao_sample_format format;
	int driver;

	ao_initialize();

	format.channels = nemoplay_get_audio_channels(play);
	format.bits = nemoplay_get_audio_samplebits(play);
	format.rate = nemoplay_get_audio_samplerate(play);
	format.byte_format = AO_FMT_NATIVE;
	format.matrix = 0;

	driver = ao_default_driver_id();
	device = ao_open_live(driver, &format, NULL);

	queue = nemoplay_get_audio_queue(play);

	while ((one = nemoplay_queue_dequeue(queue)) != NULL) {
		ao_play(device, (char *)one->data, one->size);

		free(one->data);

		nemoplay_queue_destroy_one(one);
	}

	ao_close(device);
	ao_shutdown();

	return NULL;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ 0 }
	};

	struct nemoplay *play;
	pthread_t thread;
	char *mediapath = NULL;
	int opt;

	while (opt = getopt_long(argc, argv, "", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			default:
				break;
		}
	}

	if (optind < argc)
		mediapath = strdup(argv[optind]);

	if (mediapath == NULL)
		return 0;

	play = nemoplay_create();
	if (play == NULL)
		goto out1;

	nemoplay_dump_media(mediapath);

	nemoplay_prepare_media(play, mediapath);

	pthread_create(&thread, NULL, nemoplay_handle_videoplay, (void *)play);
	pthread_create(&thread, NULL, nemoplay_handle_audioplay, (void *)play);

	nemoplay_decode_media(play);

	pthread_join(thread, NULL);

	nemoplay_destroy(play);

out1:
	free(mediapath);

	return 0;
}
