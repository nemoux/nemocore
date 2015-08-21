#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <float.h>

#include <minipalm.h>
#include <showhelper.h>
#include <geometryhelper.h>
#include <nemomisc.h>

struct minipalm *minishell_palm_create(void)
{
	struct minipalm *palm;

	palm = (struct minipalm *)malloc(sizeof(struct minipalm));
	if (palm == NULL)
		return NULL;
	memset(palm, 0, sizeof(struct minipalm));

	return palm;
}

void minishell_palm_destroy(struct minipalm *palm)
{
	free(palm);
}

void minishell_palm_prepare(struct minishell *mini, struct minigrab *grab)
{
}

void minishell_palm_update(struct minishell *mini, struct minigrab *grab)
{
	struct nemoshow *show = mini->show;

	if (grab->type == MINISHELL_NORMAL_GRAB) {
		struct minigrab *fingers[5];
		struct minigrab *other;
		double maxd = FLT_MIN, d0, d1;
		double maxx = grab->x;
		double maxy = grab->y;
		int nfingers = 0;
		int i;

		nemolist_for_each(other, &mini->grab_list, link) {
			if (grab == other || other->type != MINISHELL_NORMAL_GRAB)
				continue;

			if (((d0 = point_get_distance(grab->x, grab->y, other->x, other->y)) < 500.0f) &&
					((d1 = point_get_distance(maxx, maxy, other->x, other->y)) < 500.0f)) {
				fingers[nfingers++] = other;

				if (d0 > maxd) {
					maxx = other->x;
					maxy = other->y;
					maxd = d0;
				}
			}

			if (nfingers >= 5)
				break;
		}

		if (nfingers >= 4) {
			struct minipalm *palm;
			uint32_t serial = ++mini->serial;

			palm = minishell_palm_create();
			palm->fingers[0] = grab;
			palm->fingers[1] = fingers[0];
			palm->fingers[2] = fingers[1];
			palm->fingers[3] = fingers[2];
			palm->fingers[4] = fingers[3];

			for (i = 0; i < 5; i++) {
				palm->fingers[i]->type = MINISHELL_PALM_GRAB;
				palm->fingers[i]->serial = serial;
				palm->fingers[i]->userdata = palm;

				nemoshow_item_set_fill_color(palm->fingers[i]->one, 0, 0, 255, 255);
				nemoshow_one_dirty(palm->fingers[i]->one, NEMOSHOW_STYLE_DIRTY);
			}
		}
	} else if (grab->type == MINISHELL_PALM_GRAB) {
		struct minipalm *palm = (struct minipalm *)grab->userdata;
		double d;
		int activated = 1;
		int i;

		for (i = 1; i < 5; i++) {
			if (point_get_distance(
						palm->fingers[0]->x, palm->fingers[0]->y,
						palm->fingers[i]->x, palm->fingers[i]->y) > 200.0f) {
				activated = 0;
			}
		}

		if (activated != 0) {
			for (i = 0; i < 5; i++) {
				nemoshow_item_set_fill_color(palm->fingers[i]->one, 255, 255, 255, 255);
				nemoshow_one_dirty(palm->fingers[i]->one, NEMOSHOW_STYLE_DIRTY);
			}
		}

		if (point_get_distance(palm->fingers[0]->x, palm->fingers[0]->y, grab->x, grab->y) > 500.0f) {
			for (i = 0; i < 5; i++) {
				palm->fingers[i]->type = MINISHELL_NORMAL_GRAB;
				palm->fingers[i]->userdata = NULL;

				nemoshow_item_set_fill_color(palm->fingers[i]->one, 255, 255, 0, 255);
				nemoshow_one_dirty(palm->fingers[i]->one, NEMOSHOW_STYLE_DIRTY);
			}

			minishell_palm_destroy(palm);
		}
	}
}

void minishell_palm_finish(struct minishell *mini, struct minigrab *grab)
{
	struct nemoshow *show = mini->show;
	struct showtransition *trans;
	struct showone *sequence;

	if (grab->type == MINISHELL_PALM_GRAB) {
		struct minipalm *palm = (struct minipalm *)grab->userdata;
		int i;

		for (i = 0; i < 5; i++) {
			palm->fingers[i]->type = MINISHELL_NORMAL_GRAB;
			palm->fingers[i]->userdata = NULL;

			nemoshow_item_set_fill_color(palm->fingers[i]->one, 255, 255, 0, 255);
			nemoshow_one_dirty(palm->fingers[i]->one, NEMOSHOW_STYLE_DIRTY);
		}

		minishell_palm_destroy(palm);
	}

	sequence = nemoshow_sequence_create_easy(show,
			nemoshow_sequence_create_frame_easy(show,
				1.0f,
				nemoshow_sequence_create_set_easy(show,
					grab->one,
					"r", "0.0",
					NULL),
				NULL),
			NULL);

	trans = nemoshow_transition_create(nemoshow_search_one(show, "ease0"), 300, 0);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(show, trans);
}
