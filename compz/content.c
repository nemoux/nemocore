#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <content.h>
#include <pointer.h>
#include <keyboard.h>
#include <touch.h>
#include <nemomisc.h>

int nemocontent_prepare(struct nemocontent *content, int nodemax)
{
	content->pcontexts = (void **)malloc(sizeof(void *) * nodemax);
	if (content->pcontexts == NULL)
		return -1;
	memset(content->pcontexts, 0, sizeof(void *) * nodemax);

	content->gcontexts = (void **)malloc(sizeof(void *) * nodemax);
	if (content->gcontexts == NULL)
		goto err1;
	memset(content->gcontexts, 0, sizeof(void *) * nodemax);

	pixman_region32_init(&content->damage);
	pixman_region32_init(&content->opaque);
	pixman_region32_init(&content->input);

	content->key_handler = NULL;
	content->button_handler = NULL;
	content->touch_handler = NULL;
	content->data = NULL;

	content->region.has_input = 0;

	return 0;

err1:
	free(content->pcontexts);

	return -1;
}

void nemocontent_finish(struct nemocontent *content)
{
	pixman_region32_fini(&content->damage);
	pixman_region32_fini(&content->opaque);
	pixman_region32_fini(&content->input);

	free(content->pcontexts);
	free(content->gcontexts);
}

void nemocontent_get_viewport_transform(struct nemocontent *content, pixman_transform_t *transform)
{
	if (content->get_viewport_transform != NULL) {
		content->get_viewport_transform(content, transform);
	}
}

int32_t nemocontent_get_buffer_scale(struct nemocontent *content)
{
	if (content->get_buffer_scale != NULL)
		return content->get_buffer_scale(content);

	return 1;
}

void nemocontent_transform_to_buffer_point(struct nemocontent *content, float sx, float sy, float *bx, float *by)
{
	if (content->transform_to_buffer_point != NULL) {
		content->transform_to_buffer_point(content, sx, sy, bx, by);
	} else {
		*bx = MIN(MAX(sx, 0), content->width);
		*by = MIN(MAX(sy, 0), content->height);
	}
}

pixman_box32_t nemocontent_transform_to_buffer_rect(struct nemocontent *content, pixman_box32_t rect)
{
	if (content->transform_to_buffer_rect != NULL) {
		return content->transform_to_buffer_rect(content, rect);
	}

	rect.x1 = MAX(rect.x1, 0);
	rect.y1 = MAX(rect.y1, 0);
	rect.x2 = MIN(rect.x2, content->width);
	rect.y2 = MIN(rect.y2, content->height);

	return rect;
}

void nemocontent_update_output(struct nemocontent *content)
{
	if (content->update_output != NULL)
		content->update_output(content);
}

void nemocontent_update_transform(struct nemocontent *content, int visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
	if (content->update_transform != NULL) {
		content->update_transform(content, visible, x, y, width, height);
	}
}

void nemocontent_update_layer(struct nemocontent *content, int visible)
{
	if (content->update_layer != NULL) {
		content->update_layer(content, visible);
	}
}

void nemocontent_update_fullscreen(struct nemocontent *content, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	if (content->update_fullscreen != NULL) {
		content->update_fullscreen(content, id, x, y, width, height);
	}
}
