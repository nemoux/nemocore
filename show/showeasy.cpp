#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdarg.h>

#include <showeasy.h>

struct showtransition *nemoshow_transition_create_easy(struct nemoshow *show, struct showone *ease, uint32_t duration, uint32_t delay, ...)
{
	struct showtransition *trans;
	struct showone *sequence;
	va_list vargs;
	const char *name;

	trans = nemoshow_transition_create(ease, duration, delay);
	if (trans == NULL)
		return NULL;

	va_start(vargs, delay);

	while ((name = va_arg(vargs, const char *)) != NULL) {
		sequence = nemoshow_search_one(show, name);
		nemoshow_update_one_expression(show, sequence);
		nemoshow_transition_attach_sequence(trans, sequence);
	}

	va_end(vargs);

	return trans;
}

void nemoshow_attach_transition_easy(struct nemoshow *show, ...)
{
	struct showtransition *ptrans = NULL;
	struct showtransition *trans;
	va_list vargs;

	va_start(vargs, show);

	while ((trans = va_arg(vargs, struct showtransition *)) != NULL) {
		if (ptrans == NULL) {
			nemoshow_attach_transition(show, trans);
		} else {
			nemoshow_transition_attach_transition(ptrans, trans);
		}

		ptrans = trans;
	}

	va_end(vargs);
}
