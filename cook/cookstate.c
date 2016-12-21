#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdarg.h>

#include <cookstate.h>
#include <nemomisc.h>

static void nemocook_state_update_none(struct cookstate *state)
{
}

static void nemocook_state_update_color_buffer(struct cookstate *state)
{
	glClearColor(
			state->u.color_buffer.r,
			state->u.color_buffer.g,
			state->u.color_buffer.b,
			state->u.color_buffer.a);
	glClear(GL_COLOR_BUFFER_BIT);
}

static void nemocook_state_update_depth_buffer(struct cookstate *state)
{
	glClearDepth(state->u.depth_buffer.d);
	glClear(GL_DEPTH_BUFFER_BIT);
}

static void nemocook_state_update_blend_enable(struct cookstate *state)
{
	glEnable(GL_BLEND);
	glBlendFunc(state->u.blend.sfactor, state->u.blend.dfactor);
}

static void nemocook_state_update_blend_separate_enable(struct cookstate *state)
{
	glEnable(GL_BLEND);
	glBlendFuncSeparate(
			state->u.blend_separate.srgb,
			state->u.blend_separate.drgb,
			state->u.blend_separate.salpha,
			state->u.blend_separate.dalpha);
}

static void nemocook_state_update_blend_disable(struct cookstate *state)
{
	glDisable(GL_BLEND);
}

static void nemocook_state_update_depth_test_enable(struct cookstate *state)
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(state->u.depth_test.func);
	glDepthMask(state->u.depth_test.mask);
}

static void nemocook_state_update_depth_test_disable(struct cookstate *state)
{
	glDisable(GL_DEPTH_TEST);
}

static void nemocook_state_update_cull_face_enable(struct cookstate *state)
{
	glEnable(GL_CULL_FACE);
	glCullFace(state->u.cull_face.mode);
}

static void nemocook_state_update_cull_face_disable(struct cookstate *state)
{
	glDisable(GL_CULL_FACE);
}

struct cookstate *nemocook_state_create(int tag, int type, ...)
{
	struct cookstate *state;
	va_list vargs;
	int enable;

	state = (struct cookstate *)malloc(sizeof(struct cookstate));
	if (state == NULL)
		return NULL;

	state->tag = tag;
	state->update = nemocook_state_update_none;

	nemolist_init(&state->link);

	va_start(vargs, type);

	if (type == NEMOCOOK_STATE_COLOR_BUFFER_TYPE) {
		state->u.color_buffer.r = va_arg(vargs, double);
		state->u.color_buffer.g = va_arg(vargs, double);
		state->u.color_buffer.b = va_arg(vargs, double);
		state->u.color_buffer.a = va_arg(vargs, double);

		state->update = nemocook_state_update_color_buffer;
	} else if (type == NEMOCOOK_STATE_DEPTH_BUFFER_TYPE) {
		state->u.depth_buffer.d = va_arg(vargs, double);

		state->update = nemocook_state_update_depth_buffer;
	} else if (type == NEMOCOOK_STATE_BLEND_TYPE) {
		enable = va_arg(vargs, int);
		if (enable != 0) {
			state->u.blend.sfactor = va_arg(vargs, int);
			state->u.blend.dfactor = va_arg(vargs, int);

			state->update = nemocook_state_update_blend_enable;
		} else {
			state->update = nemocook_state_update_blend_disable;
		}
	} else if (type == NEMOCOOK_STATE_BLEND_SEPARATE_TYPE) {
		enable = va_arg(vargs, int);
		if (enable != 0) {
			state->u.blend_separate.srgb = va_arg(vargs, int);
			state->u.blend_separate.drgb = va_arg(vargs, int);
			state->u.blend_separate.salpha = va_arg(vargs, int);
			state->u.blend_separate.dalpha = va_arg(vargs, int);

			state->update = nemocook_state_update_blend_separate_enable;
		} else {
			state->update = nemocook_state_update_blend_disable;
		}
	} else if (type == NEMOCOOK_STATE_DEPTH_TEST_TYPE) {
		enable = va_arg(vargs, int);
		if (enable != 0) {
			state->u.depth_test.func = va_arg(vargs, int);
			state->u.depth_test.mask = va_arg(vargs, int);

			state->update = nemocook_state_update_depth_test_enable;
		} else {
			state->update = nemocook_state_update_depth_test_disable;
		}
	} else if (type == NEMOCOOK_STATE_CULL_FACE_TYPE) {
		enable = va_arg(vargs, int);
		if (enable != 0) {
			state->u.cull_face.mode = va_arg(vargs, int);

			state->update = nemocook_state_update_cull_face_enable;
		} else {
			state->update = nemocook_state_update_cull_face_disable;
		}
	}

	va_end(vargs);

	return state;
}

void nemocook_state_destroy(struct cookstate *state)
{
	nemolist_remove(&state->link);

	free(state);
}
