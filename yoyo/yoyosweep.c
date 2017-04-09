#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <yoyosweep.h>
#include <yoyoactor.h>
#include <yoyoregion.h>
#include <yoyoone.h>
#include <nemoyoyo.h>
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
	float rs, rn;

	tex = yoyo->sweeps[random_get_int(0, yoyo->nsweeps - 1)];

	rs = random_get_double(sweep->minimum_range, sweep->maximum_range);
	rn = random_get_double(0.0f, M_PI * 2.0f);

	one = nemoyoyo_one_create();
	nemoyoyo_one_set_tx(one,
			nemoaction_tap_get_tx(tap));
	nemoyoyo_one_set_ty(one,
			nemoaction_tap_get_ty(tap));
	nemoyoyo_one_set_width(one,
			nemocook_texture_get_width(tex));
	nemoyoyo_one_set_height(one,
			nemocook_texture_get_height(tex));
	nemoyoyo_one_set_sx(one, sweep->feedback_sx0);
	nemoyoyo_one_set_sy(one, sweep->feedback_sy0);
	nemoyoyo_one_set_alpha(one, sweep->feedback_alpha0);
	nemoyoyo_one_set_texture(one, tex);
	nemoyoyo_attach_one(yoyo, one);

	trans = nemotransition_create(8,
			NEMOEASE_CUBIC_OUT_TYPE,
			random_get_int(sweep->minimum_duration, sweep->maximum_duration),
			0);
	nemoyoyo_one_transition_set_tx(trans, 0, one);
	nemoyoyo_one_transition_set_ty(trans, 1, one);
	nemoyoyo_one_transition_set_sx(trans, 2, one);
	nemoyoyo_one_transition_set_sy(trans, 3, one);
	nemoyoyo_one_transition_set_alpha(trans, 4, one);
	nemoyoyo_one_transition_check_destroy(trans, one);
	nemotransition_set_target(trans, 0, 1.0f,
			nemoaction_tap_get_tx(tap) + cos(rn) * rs);
	nemotransition_set_target(trans, 1, 1.0f,
			nemoaction_tap_get_ty(tap) + sin(rn) * rs);
	nemotransition_set_target(trans, 2, 1.0f, sweep->feedback_sx1);
	nemotransition_set_target(trans, 3, 1.0f, sweep->feedback_sy1);
	nemotransition_set_target(trans, 4, 1.0f, sweep->feedback_alpha1);
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
		if (nemoaction_tap_get_duration(tap) > sweep->actor_duration && nemoaction_tap_get_distance(tap) > sweep->actor_distance) {
			struct json_object *jobj;

			jobj = nemojson_search_object(yoyo->config, 0, 1, "items");
			if (jobj != NULL) {
				float cx = nemoaction_tap_get_tx(tap);
				float cy = nemoaction_tap_get_ty(tap);

				if (nemoyoyo_overlap_actor(yoyo, cx, cy) == 0) {
					struct yoyoactor *actor;
					struct yoyoregion *region;
					struct json_object *cobj;
					float rs, rn;
					int i;

					region = nemoyoyo_search_region(yoyo, cx, cy);

					for (i = 0; i < nemojson_array_get_length(jobj); i++) {
						cobj = nemojson_array_get_object(jobj, i);

						rs = random_get_double(160.0f, 180.0f);
						rn = random_get_double(0.0f, M_PI * 2.0f);

						actor = nemoyoyo_actor_create(yoyo);
						nemoyoyo_actor_set_json_object(actor, cobj);
						nemoyoyo_actor_set_lifetime(actor, 1800);
						nemoyoyo_actor_set_hidetime(actor, 800);
						nemoyoyo_actor_dispatch(actor,
								cx,
								cy,
								cx + cos(rn) * rs,
								cy + sin(rn) * rs,
								region == NULL ? 0.0f : nemoyoyo_region_get_rotate(region));
					}
				}
			}
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
