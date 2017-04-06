#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <yoyosweep.h>
#include <yoyoactor.h>
#include <nemoyoyo.h>
#include <yoyoone.h>
#include <nemomisc.h>

static void nemoyoyo_sweep_dispatch_transition_done(struct nemotransition *trans, void *data)
{
	struct yoyoone *one = (struct yoyoone *)data;

	nemoyoyo_one_destroy(one);
}

static void nemoyoyo_sweep_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct yoyosweep *sweep = (struct yoyosweep *)data;
	struct nemoyoyo *yoyo = sweep->yoyo;
	struct actiontap *tap = sweep->tap;
	struct nemotransition *trans;
	struct yoyoone *one;
	struct cooktex *tex;

	tex = yoyo->sweeps[random_get_int(0, yoyo->nsweeps - 1)];

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

	trans = nemotransition_create(8,
			NEMOEASE_CUBIC_INOUT_TYPE,
			random_get_int(sweep->minimum_duration, sweep->maximum_duration),
			0);
	nemoyoyo_one_transition_set_sx(trans, 0, one);
	nemoyoyo_one_transition_set_sy(trans, 1, one);
	nemoyoyo_one_transition_set_alpha(trans, 2, one);
	nemoyoyo_one_transition_check(trans, one);
	nemotransition_set_target(trans, 0, 1.0f, 0.0f);
	nemotransition_set_target(trans, 1, 1.0f, 0.0f);
	nemotransition_set_target(trans, 2, 1.0f, 0.0f);
	nemotransition_set_dispatch_done(trans, nemoyoyo_sweep_dispatch_transition_done);
	nemotransition_set_userdata(trans, one);
	nemotransition_group_attach_transition(yoyo->transitions, trans);

	nemocanvas_dispatch_frame(yoyo->canvas);

	nemotimer_set_timeout(sweep->timer,
			random_get_int(sweep->minimum_interval, sweep->maximum_interval));
}

static int nemoyoyo_sweep_dispatch_tap_event(struct nemoaction *action, struct actiontap *tap, uint32_t event)
{
	struct nemoyoyo *yoyo = (struct nemoyoyo *)nemoaction_get_userdata(action);
	struct yoyosweep *sweep = (struct yoyosweep *)nemoaction_tap_get_userdata(tap);

	if (event & NEMOACTION_TAP_DOWN_EVENT) {
		sweep->timer = nemotimer_create(nemotool_get_instance());
		nemotimer_set_callback(sweep->timer, nemoyoyo_sweep_dispatch_timer);
		nemotimer_set_userdata(sweep->timer, sweep);

		nemotimer_set_timeout(sweep->timer,
				random_get_int(sweep->minimum_interval, sweep->maximum_interval));
	} else if (event & NEMOACTION_TAP_MOTION_EVENT) {
	} else if (event & NEMOACTION_TAP_UP_EVENT) {
		if (nemoaction_tap_get_duration(tap) > sweep->actor_duration &&
				nemoaction_tap_get_distance(tap) > sweep->actor_distance) {
			struct yoyoactor *actor;

			actor = nemoyoyo_actor_create(yoyo);
			nemoyoyo_actor_set_json_object(actor,
					nemojson_search_object(yoyo->config, 0, 1, "menu"));
			nemoyoyo_actor_set_lifetime(actor, 1800);
			nemoyoyo_actor_set_hidetime(actor, 800);
			nemoyoyo_actor_dispatch(actor, tap);
		}

		nemoyoyo_sweep_destroy(sweep);
	}

	return 0;
}

struct yoyosweep *nemoyoyo_sweep_create(struct nemoyoyo *yoyo)
{
	struct yoyosweep *sweep;

	sweep = (struct yoyosweep *)malloc(sizeof(struct yoyosweep));
	if (sweep == NULL)
		return NULL;
	memset(sweep, 0, sizeof(struct yoyosweep));

	sweep->yoyo = yoyo;

	return sweep;
}

void nemoyoyo_sweep_destroy(struct yoyosweep *sweep)
{
	if (sweep->timer != NULL)
		nemotimer_destroy(sweep->timer);

	free(sweep);
}

int nemoyoyo_sweep_dispatch(struct yoyosweep *sweep, struct actiontap *tap)
{
	sweep->tap = tap;

	nemoaction_tap_set_callback(tap, nemoyoyo_sweep_dispatch_tap_event);
	nemoaction_tap_set_userdata(tap, sweep);

	nemoaction_tap_dispatch_event(
			nemoaction_tap_get_action(tap),
			tap,
			NEMOACTION_TAP_DOWN_EVENT);

	return 0;
}
