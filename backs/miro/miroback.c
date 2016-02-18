#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <pthread.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#include <fftw3.h>

#include <nemotool.h>
#include <miroback.h>
#include <mirotap.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

#define NEMOMIRO_PA_SAMPLING_RATE				(44100)
#define NEMOMIRO_PA_SAMPLING_CHANNELS		(2)
#define NEMOMIRO_PA_SAMPLING_FRAMES			(30)
#define NEMOMIRO_PA_BUFFER_SIZE					(NEMOMIRO_PA_SAMPLING_RATE / NEMOMIRO_PA_SAMPLING_FRAMES)
#define NEMOMIRO_PA_UPPER_FREQUENCY			(3520.0f)
#define NEMOMIRO_PA_VOLUME_DECIBELS			(100.0f)

struct miromice {
	struct miroback *miro;

	struct showone *one;

	int32_t c0, r0;
	int32_t c1, r1;
};

static void nemoback_miro_dispatch_show(struct miroback *miro, uint32_t duration, uint32_t interval);
static void nemoback_miro_dispatch_hide(struct miroback *miro, uint32_t duration, uint32_t interval);

static int nemoback_miro_dispatch_tap_grab(struct nemoshow *show, struct showgrab *grab, void *event)
{
	struct mirotap *tap = (struct mirotap *)nemoshow_grab_get_userdata(grab);
	struct miroback *miro = tap->miro;

	if (nemoshow_event_is_down(show, event)) {
		nemoback_mirotap_down(miro, tap, nemoshow_event_get_x(event), nemoshow_event_get_y(event));

		nemoshow_dispatch_frame(show);
	} else if (nemoshow_event_is_motion(show, event)) {
		nemoback_mirotap_motion(miro, tap, nemoshow_event_get_x(event), nemoshow_event_get_y(event));

		nemoshow_dispatch_frame(show);
	} else if (nemoshow_event_is_up(show, event)) {
		nemoback_mirotap_up(miro, tap, nemoshow_event_get_x(event), nemoshow_event_get_y(event));

		nemoshow_dispatch_frame(show);

		nemoshow_grab_destroy(grab);

		return 0;
	}

	return 1;
}

static void nemoback_miro_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, void *event)
{
	struct miroback *miro = (struct miroback *)nemoshow_get_userdata(show);

	if (nemoshow_event_is_down(show, event)) {
		struct showgrab *grab;
		struct mirotap *tap;

		tap = nemoback_mirotap_create(miro);

		grab = nemoshow_grab_create(show, event, nemoback_miro_dispatch_tap_grab);
		nemoshow_grab_set_userdata(grab, tap);
		nemoshow_grab_check_signal(grab, &tap->destroy_signal);
		nemoshow_dispatch_grab(show, event);
	}
}

static void nemoback_miro_dispatch_mice_destroy_done(void *data)
{
	struct miromice *mice = (struct miromice *)data;
	struct miroback *miro = mice->miro;

	miro->nmices--;

	nemoshow_one_destroy(mice->one);

	free(mice);
}

static void nemoback_miro_dispatch_mice_transition_done(void *data)
{
	struct miromice *mice = (struct miromice *)data;
	struct miroback *miro = mice->miro;
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0;

	if (miro->is_sleeping != 0 ||
			mice->c1 <= 0 || mice->c1 >= miro->columns ||
			mice->r1 <= 0 || mice->r1 >= miro->rows) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, mice->one);
		nemoshow_sequence_set_dattr(set0, "r", 0.0f, NEMOSHOW_SHAPE_DIRTY);

		sequence = nemoshow_sequence_create_easy(miro->show,
				nemoshow_sequence_create_frame_easy(miro->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(miro->ease1, 1000, 0);
		nemoshow_transition_set_dispatch_done(trans, nemoback_miro_dispatch_mice_destroy_done);
		nemoshow_transition_set_userdata(trans, mice);
		nemoshow_transition_check_one(trans, mice->one);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);
	} else {
		int dir = random_get_int(0, 4);

		mice->c0 = mice->c1;
		mice->r0 = mice->r1;

		if (dir == 0) {
			mice->c1 = mice->c0 + 1;
			mice->r1 = mice->r1;
		} else if (dir == 1) {
			mice->c1 = mice->c0 - 1;
			mice->r1 = mice->r1;
		} else if (dir == 2) {
			mice->c1 = mice->c0;
			mice->r1 = mice->r1 + 1;
		} else if (dir == 3) {
			mice->c1 = mice->c0;
			mice->r1 = mice->r1 - 1;
		}

		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, mice->one);
		nemoshow_sequence_set_dattr(set0, "x", mice->c1 * (miro->width / miro->columns), NEMOSHOW_SHAPE_DIRTY);
		nemoshow_sequence_set_dattr(set0, "y", mice->r1 * (miro->height / miro->rows), NEMOSHOW_SHAPE_DIRTY);

		sequence = nemoshow_sequence_create_easy(miro->show,
				nemoshow_sequence_create_frame_easy(miro->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(miro->ease2, 2400, 0);
		nemoshow_transition_set_dispatch_done(trans, nemoback_miro_dispatch_mice_transition_done);
		nemoshow_transition_set_userdata(trans, mice);
		nemoshow_transition_check_one(trans, mice->one);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);
	}
}

static int nemoback_miro_shoot_mice(struct miroback *miro)
{
	struct miromice *mice;
	struct showone *one;
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0;

	mice = (struct miromice *)malloc(sizeof(struct miromice));
	if (mice == NULL)
		return -1;
	memset(mice, 0, sizeof(struct miromice));

	mice->miro = miro;

	mice->c0 = random_get_int(1, miro->columns);
	mice->r0 = random_get_int(1, miro->rows);
	mice->c1 = mice->c0;
	mice->r1 = mice->r0;

	mice->one = one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
	nemoshow_attach_one(miro->show, one);
	nemoshow_one_attach(miro->canvas, one);
	nemoshow_item_set_x(one, mice->c0 * (miro->width / miro->columns));
	nemoshow_item_set_y(one, mice->r0 * (miro->height / miro->rows));
	nemoshow_item_set_r(one, 0.0f);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_filter(one, miro->solid);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, one);
	nemoshow_sequence_set_dattr(set0, "r", 5.0f, NEMOSHOW_SHAPE_DIRTY);

	sequence = nemoshow_sequence_create_easy(miro->show,
			nemoshow_sequence_create_frame_easy(miro->show,
				1.0f, set0, NULL),
			NULL);

	trans = nemoshow_transition_create(miro->ease1, 1000, 0);
	nemoshow_transition_set_dispatch_done(trans, nemoback_miro_dispatch_mice_transition_done);
	nemoshow_transition_set_userdata(trans, mice);
	nemoshow_transition_check_one(trans, one);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(miro->show, trans);

	return 0;
}

static int nemoback_miro_shoot_box(struct miroback *miro)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0, *set1;
	int index;
	int i;

	for (i = 0; i < 8; i++) {
		index = random_get_int(0, miro->columns * miro->rows);

		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, miro->bones[index]);
		nemoshow_sequence_set_dattr(set0, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);

		set1 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set1, miro->bones[index]);
		nemoshow_sequence_set_dattr(set1, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);

		sequence = nemoshow_sequence_create_easy(miro->show,
				nemoshow_sequence_create_frame_easy(miro->show,
					0.5f, set0, NULL),
				nemoshow_sequence_create_frame_easy(miro->show,
					1.0f, set1, NULL),
				NULL);

		trans = nemoshow_transition_create(miro->ease1, 1500, i * 300);
		nemoshow_transition_check_one(trans, miro->bones[index]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);
	}

	return 0;
}

static void nemoback_miro_dispatch_timer_event(struct nemotimer *timer, void *data)
{
	struct miroback *miro = (struct miroback *)data;

	if (miro->is_sleeping == 0 && miro->nmices < miro->mmices) {
		nemoback_miro_shoot_mice(miro);

		miro->nmices++;

		nemoshow_dispatch_frame(miro->show);
	}

	nemotimer_set_timeout(miro->timer, 3000);
}

static void *nemoback_miro_dispatch_pulse_monitor_thread(void *data)
{
	static const pa_sample_spec psample = {
		.format = PA_SAMPLE_FLOAT32LE,
		.rate = NEMOMIRO_PA_SAMPLING_RATE,
		.channels = NEMOMIRO_PA_SAMPLING_CHANNELS
	};

	struct miroback *miro = (struct miroback *)data;
	pa_simple *psimple;
	double pin[NEMOMIRO_PA_BUFFER_SIZE];
	fftw_complex pout[NEMOMIRO_PA_BUFFER_SIZE];
	fftw_plan pplan;
	float window[NEMOMIRO_PA_BUFFER_SIZE];
	float buffer[NEMOMIRO_PA_BUFFER_SIZE * NEMOMIRO_PA_SAMPLING_CHANNELS];
	double scale = 2.0f / NEMOMIRO_PA_BUFFER_SIZE;
	double power, re, im;
	int nodes = miro->columns * miro->rows / 2;
	int units = ceil(NEMOMIRO_PA_UPPER_FREQUENCY / (NEMOMIRO_PA_SAMPLING_FRAMES * nodes));
	int error;
	int db;
	int i, j, s;

	psimple = pa_simple_new(NULL, "miro", PA_STREAM_RECORD, miro->snddev, "miro", &psample, NULL, NULL, &error);
	if (psimple == NULL) {
		nemolog_error("MIRO", "can't create pulseaudio: %s\n", pa_strerror(error));
		return NULL;
	}

	pplan = fftw_plan_dft_r2c_1d(NEMOMIRO_PA_BUFFER_SIZE, pin, pout, FFTW_MEASURE);

	for (i = 0; i < NEMOMIRO_PA_BUFFER_SIZE; i++) {
		window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (NEMOMIRO_PA_BUFFER_SIZE - 1.0f)));
	}

	while (1) {
		if (pa_simple_read(psimple, buffer, sizeof(buffer), &error) < 0) {
			nemolog_error("MIRO", "can't read pulseaudio: %s\n", pa_strerror(error));
			break;
		}

		for (i = 0; i < NEMOMIRO_PA_BUFFER_SIZE; i++)
			pin[i] = (double)(window[i] * buffer[i * 2 + 0]);

		fftw_execute(pplan);

		pthread_mutex_lock(&miro->plock);

		for (i = 0, s = 0; i < nodes; i++) {
			power = 0.0f;

			for (j = 0; j < units && s < NEMOMIRO_PA_BUFFER_SIZE; j++, s++) {
				re = pout[i][0] * scale;
				im = pout[i][1] * scale;

				power += re * re + im * im;
			}

			power *= (1.0f / units);
			power = power < 1e-15 ? 1e-15 : power;

			db = NEMOMIRO_PA_VOLUME_DECIBELS + 10.0f * log10(power);
			db = db > NEMOMIRO_PA_VOLUME_DECIBELS ? NEMOMIRO_PA_VOLUME_DECIBELS : db;
			db = db < 0 ? 0 : db;

			miro->nodes1[i] = db;
		}

		for (i = 0; i < NEMOMIRO_PA_BUFFER_SIZE; i++)
			pin[i] = (double)(window[i] * buffer[i * 2 + 1]);

		fftw_execute(pplan);

		for (i = 0, s = 0; i < nodes; i++) {
			power = 0.0f;

			for (j = 0; j < units && s < NEMOMIRO_PA_BUFFER_SIZE; j++, s++) {
				re = pout[i][0] * scale;
				im = pout[i][1] * scale;

				power += re * re + im * im;
			}

			power *= (1.0f / units);
			power = power < 1e-15 ? 1e-15 : power;

			db = NEMOMIRO_PA_VOLUME_DECIBELS + 10.0f * log10(power);
			db = db > NEMOMIRO_PA_VOLUME_DECIBELS ? NEMOMIRO_PA_VOLUME_DECIBELS : db;
			db = db < 0 ? 0 : db;

			miro->nodes1[miro->columns * miro->rows - i - 1] = db;
		}

		pthread_mutex_unlock(&miro->plock);
	}

	return NULL;
}

static void nemoback_miro_dispatch_pulse_timer_event(struct nemotimer *timer, void *data)
{
	struct miroback *miro = (struct miroback *)data;
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *frame;
	struct showone *set0;
	int i;

	if (miro->is_sleeping == 0) {
		frame = nemoshow_sequence_create_frame();
		nemoshow_sequence_set_timing(frame, 1.0f);

		pthread_mutex_lock(&miro->plock);

		for (i = 0; i < miro->columns * miro->rows; i++) {
			if (miro->nodes0[i] != miro->nodes1[i]) {
				set0 = nemoshow_sequence_create_set();
				nemoshow_sequence_set_source(set0, miro->bones[i]);
				nemoshow_sequence_set_dattr(set0, "alpha", (double)miro->nodes1[i] / NEMOMIRO_PA_VOLUME_DECIBELS, NEMOSHOW_STYLE_DIRTY);

				nemoshow_one_attach(frame, set0);

				miro->nodes0[i] = miro->nodes1[i];
			}
		}

		pthread_mutex_unlock(&miro->plock);

		sequence = nemoshow_sequence_create();
		nemoshow_one_attach(sequence, frame);

		trans = nemoshow_transition_create(miro->ease2, 150, 0);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);

		nemoshow_dispatch_frame(miro->show);
	}

	nemotimer_set_timeout(timer, 100);
}

static int nemoback_miro_dispatch_pulse_monitor(struct miroback *miro)
{
	struct nemotimer *timer;
	pthread_t th;

	pthread_mutex_init(&miro->plock, NULL);

	pthread_create(&th, NULL, nemoback_miro_dispatch_pulse_monitor_thread, (void *)miro);

	miro->ptimer = timer = nemotimer_create(miro->tool);
	nemotimer_set_callback(timer, nemoback_miro_dispatch_pulse_timer_event);
	nemotimer_set_userdata(timer, miro);

	nemotimer_set_timeout(timer, 100);

	return 0;
}

static void nemoback_miro_dispatch_show_transition_done(void *userdata)
{
	struct miroback *miro = (struct miroback *)userdata;

	nemotimer_set_timeout(miro->timer, 1000);

	nemoback_miro_dispatch_pulse_monitor(miro);

	nemoshow_set_dispatch_transition_done(miro->show, NULL, NULL);
}

static void nemoback_miro_dispatch_show(struct miroback *miro, uint32_t duration, uint32_t interval)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0;
	int i;

	for (i = 0; i <= miro->columns; i++) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, miro->cones[i]);
		nemoshow_sequence_set_dattr(set0, "height", miro->height, NEMOSHOW_SHAPE_DIRTY);

		sequence = nemoshow_sequence_create_easy(miro->show,
				nemoshow_sequence_create_frame_easy(miro->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(miro->ease1, duration, i * interval);
		nemoshow_transition_check_one(trans, miro->cones[i]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);
	}

	for (i = 0; i <= miro->rows; i++) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, miro->rones[i]);
		nemoshow_sequence_set_dattr(set0, "width", miro->width, NEMOSHOW_SHAPE_DIRTY);

		sequence = nemoshow_sequence_create_easy(miro->show,
				nemoshow_sequence_create_frame_easy(miro->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(miro->ease1, duration, i * interval);
		nemoshow_transition_check_one(trans, miro->rones[i]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);
	}

	nemoshow_set_dispatch_transition_done(miro->show, nemoback_miro_dispatch_show_transition_done, miro);

	nemoshow_dispatch_frame(miro->show);
}

static void nemoback_miro_dispatch_hide(struct miroback *miro, uint32_t duration, uint32_t interval)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0;
	int i;

	for (i = 0; i <= miro->columns; i++) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, miro->cones[i]);
		nemoshow_sequence_set_dattr(set0, "height", 0.0f, NEMOSHOW_SHAPE_DIRTY);

		sequence = nemoshow_sequence_create_easy(miro->show,
				nemoshow_sequence_create_frame_easy(miro->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(miro->ease1, duration, i * interval);
		nemoshow_transition_check_one(trans, miro->cones[i]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);
	}

	for (i = 0; i <= miro->rows; i++) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, miro->rones[i]);
		nemoshow_sequence_set_dattr(set0, "width", 0.0f, NEMOSHOW_SHAPE_DIRTY);

		sequence = nemoshow_sequence_create_easy(miro->show,
				nemoshow_sequence_create_frame_easy(miro->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(miro->ease1, duration, i * interval);
		nemoshow_transition_check_one(trans, miro->rones[i]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);
	}

	nemoshow_dispatch_frame(miro->show);
}

static void nemoback_miro_dispatch_canvas_fullscreen(struct nemoshow *show, int32_t active, int32_t opaque)
{
	struct miroback *miro = (struct miroback *)nemoshow_get_userdata(show);

	if (active == 0)
		miro->is_sleeping = 0;
	else
		miro->is_sleeping = 1;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",			required_argument,			NULL,		'w' },
		{ "height",			required_argument,			NULL,		'h' },
		{ "columns",		required_argument,			NULL,		'c' },
		{ "rows",				required_argument,			NULL,		'r' },
		{ "mices",			required_argument,			NULL,		'm' },
		{ "tapsize",		required_argument,			NULL,		't' },
		{ "sounddev",		required_argument,			NULL,		's' },
		{ "log",				required_argument,			NULL,		'o' },
		{ 0 }
	};

	struct miroback *miro;
	struct nemotool *tool;
	struct nemotimer *timer;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *blur;
	struct showone *ease;
	struct showone *one;
	int32_t width = 1920;
	int32_t height = 1080;
	int32_t columns = 16;
	int32_t rows = 8;
	int32_t mices = 16;
	float tapsize = 70.0f;
	char *snddev = NULL;
	int opt;
	int i;

	nemolog_set_file(2);

	while (opt = getopt_long(argc, argv, "w:h:c:r:m:t:s:o:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'c':
				columns = strtoul(optarg, NULL, 10);
				break;

			case 'r':
				rows = strtoul(optarg, NULL, 10);
				break;

			case 'm':
				mices = strtoul(optarg, NULL, 10);
				break;

			case 't':
				tapsize = strtod(optarg, NULL);
				break;

			case 's':
				snddev = strdup(optarg);
				break;

			case 'o':
				nemolog_open_socket(optarg);
				break;

			default:
				break;
		}
	}

	miro = (struct miroback *)malloc(sizeof(struct miroback));
	if (miro == NULL)
		return -1;
	memset(miro, 0, sizeof(struct miroback));

	miro->nodes0 = (int32_t *)malloc(sizeof(int32_t) * columns * rows);
	if (miro->nodes0 == NULL)
		return -1;
	memset(miro->nodes0, 0, sizeof(int32_t) * columns * rows);

	miro->nodes1 = (int32_t *)malloc(sizeof(int32_t) * columns * rows);
	if (miro->nodes1 == NULL)
		return -1;
	memset(miro->nodes1, 0, sizeof(int32_t) * columns * rows);

	nemolist_init(&miro->tap_list);

	miro->width = width;
	miro->height = height;

	miro->columns = columns;
	miro->rows = rows;

	miro->nmices = 0;
	miro->mmices = mices;

	miro->tapsize = tapsize;

	miro->snddev = snddev;

	miro->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err1;
	nemotool_connect_wayland(tool, NULL);

	miro->timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemoback_miro_dispatch_timer_event);
	nemotimer_set_userdata(timer, miro);

	miro->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err2;
	nemoshow_set_dispatch_fullscreen(show, nemoback_miro_dispatch_canvas_fullscreen);
	nemoshow_set_userdata(show, miro);

	nemoshow_view_set_layer(show, "background");
	nemoshow_view_set_input(show, "touch");
	nemoshow_view_put_sound(show);
	nemoshow_view_set_opaque(show, 0, 0, width, height);

	miro->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_attach_one(show, scene);

	miro->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 255.0f);
	nemoshow_canvas_set_alpha(canvas, 0.0f);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	miro->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemoback_miro_dispatch_canvas_event);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);
	
	miro->ease0 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_CUBIC_INOUT_TYPE);

	miro->ease1 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_CUBIC_OUT_TYPE);

	miro->ease2 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_LINEAR_TYPE);

	miro->inner = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "inner", 3.0f);

	miro->outer = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "outer", 3.0f);

	miro->solid = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "solid", 5.0f);

	miro->cones = (struct showone **)malloc(sizeof(struct showone *) * (columns + 1));
	miro->rones = (struct showone **)malloc(sizeof(struct showone *) * (rows + 1));
	miro->bones = (struct showone **)malloc(sizeof(struct showone *) * (columns * rows));

	for (i = 0; i < columns * rows; i++) {
		miro->bones[i] = one = nemoshow_item_create(NEMOSHOW_RECT_ITEM);
		nemoshow_attach_one(show, one);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, (i % columns) * (width / columns));
		nemoshow_item_set_y(one, (i / columns) * (height / rows));
		nemoshow_item_set_width(one, width / columns);
		nemoshow_item_set_height(one, height / rows);
		nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_alpha(one, 0.0f);
	}

	for (i = 0; i <= columns; i++) {
		miro->cones[i] = one = nemoshow_item_create(NEMOSHOW_LINE_ITEM);
		nemoshow_attach_one(show, one);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, i * (width / columns));
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, i * (width / columns));
		nemoshow_item_set_height(one, 0.0f);
		nemoshow_item_set_stroke_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_stroke_width(one, 2.0f);
		nemoshow_item_set_filter(one, miro->solid);
		nemoshow_item_set_alpha(one, 0.5f);
	}

	for (i = 0; i <= rows; i++) {
		miro->rones[i] = one = nemoshow_item_create(NEMOSHOW_LINE_ITEM);
		nemoshow_attach_one(show, one);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, i * (height / rows));
		nemoshow_item_set_width(one, 0.0f);
		nemoshow_item_set_height(one, i * (height / rows));
		nemoshow_item_set_stroke_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_stroke_width(one, 2.0f);
		nemoshow_item_set_filter(one, miro->solid);
		nemoshow_item_set_alpha(one, 0.5f);
	}

	nemoback_miro_dispatch_show(miro, 1800, 100);

	nemotool_run(tool);

err3:
	nemoshow_destroy_view(show);

err2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err1:
	free(miro->nodes0);
	free(miro->nodes1);
	free(miro);

	return 0;
}
