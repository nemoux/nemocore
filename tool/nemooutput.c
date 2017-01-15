#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-client.h>
#include <wayland-nemo-shell-client-protocol.h>

#include <nemotool.h>
#include <nemooutput.h>
#include <nemomisc.h>

static void output_handle_geometry(
		void *data,
		struct wl_output *_output,
		int x, int y,
		int physical_width, int physical_height,
		int subpixel,
		const char *make,
		const char *model,
		int transform)
{
	struct nemooutput *output = (struct nemooutput *)data;

	output->x = x;
	output->y = y;

	output->mmwidth = physical_width;
	output->mmheight = physical_height;

	output->transform = transform;

	if (output->make != NULL)
		free(output->make);
	output->make = strdup(make);

	if (output->model != NULL)
		free(output->model);
	output->model = strdup(model);
}

static void output_handle_mode(
		void *data,
		struct wl_output *_output,
		uint32_t flags,
		int width, int height,
		int refresh)
{
	struct nemooutput *output = (struct nemooutput *)data;

	if (flags & WL_OUTPUT_MODE_CURRENT) {
		output->width = width;
		output->height = height;

		output->refresh = refresh;
	}
}

static void output_handle_done(void *data, struct wl_output *_output)
{
	struct nemooutput *output = (struct nemooutput *)data;
}

static void output_handle_scale(void *data, struct wl_output *_output, int32_t scale)
{
	struct nemooutput *output = (struct nemooutput *)data;

	output->scale = scale;
}

static const struct wl_output_listener output_listener = {
	output_handle_geometry,
	output_handle_mode,
	output_handle_done,
	output_handle_scale
};

struct nemooutput *nemooutput_create(struct nemotool *tool, uint32_t id)
{
	struct nemooutput *output;

	output = (struct nemooutput *)malloc(sizeof(struct nemooutput));
	if (output == NULL)
		return NULL;
	memset(output, 0, sizeof(struct nemooutput));

	nemolist_init(&output->link);

	output->tool = tool;
	output->id = id;
	output->scale = 1;

	output->output = wl_registry_bind(tool->registry, id, &wl_output_interface, 2);
	wl_output_add_listener(output->output, &output_listener, output);

	return output;
}

void nemooutput_destroy(struct nemooutput *output)
{
	if (output->tool->display != NULL)
		wl_output_destroy(output->output);

	nemolist_remove(&output->link);

	if (output->make != NULL)
		free(output->make);
	if (output->model != NULL)
		free(output->model);

	free(output);
}

int nemooutput_register(struct nemotool *tool, uint32_t id)
{
	struct nemooutput *output;

	output = nemooutput_create(tool, id);
	if (output == NULL)
		return -1;

	nemolist_insert(&tool->output_list, &output->link);

	return 0;
}

void nemooutput_unregister(struct nemotool *tool, uint32_t id)
{
	struct nemooutput *output;

	nemolist_for_each(output, &tool->output_list, link) {
		if (output->id == id) {
			nemooutput_destroy(output);

			break;
		}
	}
}

struct nemooutput *nemooutput_find(struct nemotool *tool, struct wl_output *_output)
{
	struct nemooutput *output;

	nemolist_for_each(output, &tool->output_list, link) {
		if (output->output == _output)
			return output;
	}

	return NULL;
}
