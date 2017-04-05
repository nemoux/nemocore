#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <yoyodotor.h>
#include <nemoyoyo.h>
#include <yoyoone.h>
#include <nemomisc.h>

static int nemoyoyo_dotor_dispatch_tap_event(struct nemoaction *action, struct actiontap *tap, uint32_t event)
{
	struct nemoyoyo *yoyo = (struct nemoyoyo *)nemoaction_get_userdata(action);
	struct yoyodotor *dotor = (struct yoyodotor *)nemoaction_tap_get_userdata(tap);

	if (event & NEMOACTION_TAP_DOWN_EVENT) {
	} else if (event & NEMOACTION_TAP_MOTION_EVENT) {
	} else if (event & NEMOACTION_TAP_UP_EVENT) {
		nemoyoyo_dotor_destroy(dotor);
	}

	return 0;
}

struct yoyodotor *nemoyoyo_dotor_create(struct nemoyoyo *yoyo, struct actiontap *tap)
{
	struct yoyodotor *dotor;

	dotor = (struct yoyodotor *)malloc(sizeof(struct yoyodotor));
	if (dotor == NULL)
		return NULL;
	memset(dotor, 0, sizeof(struct yoyodotor));

	dotor->yoyo = yoyo;
	dotor->tap = tap;

	nemoaction_tap_set_callback(tap, nemoyoyo_dotor_dispatch_tap_event);
	nemoaction_tap_set_userdata(tap, dotor);

	return dotor;
}

void nemoyoyo_dotor_destroy(struct yoyodotor *dotor)
{
	if (dotor->timer != NULL)
		nemotimer_destroy(dotor->timer);

	free(dotor);
}

static void nemoyoyo_dotor_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct yoyodotor *dotor = (struct yoyodotor *)data;
	struct nemoyoyo *yoyo = dotor->yoyo;
	struct actiontap *tap = dotor->tap;
	struct yoyoone *one;
	struct cooktex *tex;

	tex = yoyo->spot.textures[random_get_int(0, yoyo->spot.ntextures - 1)];

	one = nemoyoyo_one_create();
	nemoyoyo_one_set_tx(one,
			nemoaction_tap_get_tx(tap));
	nemoyoyo_one_set_ty(one,
			nemoaction_tap_get_ty(tap));
	nemoyoyo_one_set_width(one,
			nemocook_texture_get_width(tex));
	nemoyoyo_one_set_height(one,
			nemocook_texture_get_height(tex));
	nemoyoyo_one_set_texture(one, tex);

	nemoyoyo_attach_one(yoyo, one);

	nemocanvas_dispatch_frame(yoyo->canvas);

	nemotimer_set_timeout(dotor->timer,
			random_get_int(dotor->minimum_interval, dotor->maximum_interval));
}

void nemoyoyo_dotor_dispatch(struct yoyodotor *dotor, struct nemotool *tool)
{
	dotor->timer = nemotimer_create(tool);
	nemotimer_set_callback(dotor->timer, nemoyoyo_dotor_dispatch_timer);
	nemotimer_set_userdata(dotor->timer, dotor);

	nemotimer_set_timeout(dotor->timer,
			random_get_int(dotor->minimum_interval, dotor->maximum_interval));

	nemoaction_tap_dispatch_event(
			nemoaction_tap_get_action(dotor->tap),
			dotor->tap,
			NEMOACTION_TAP_DOWN_EVENT);
}
