#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-text-server-protocol.h>
#include <wayland-input-method-server-protocol.h>

#include <textbackend.h>
#include <textinput.h>
#include <inputmethod.h>
#include <compz.h>
#include <canvas.h>
#include <view.h>
#include <seat.h>
#include <keyboard.h>
#include <task.h>
#include <nemomisc.h>
#include <nemolog.h>

static void textbackend_launch_input_method(struct textbackend *textbackend);

static void textbackend_cleanup_input_method(struct nemotask *task, int status)
{
	struct textbackend *textbackend = (struct textbackend *)container_of(task, struct textbackend, inputmethod.task);
	uint32_t time;

	textbackend->inputmethod.task.pid = 0;
	textbackend->inputmethod.client = NULL;

	time = time_current_msecs();
	if (time - textbackend->inputmethod.deathstamp > 10000) {
		textbackend->inputmethod.deathstamp = time;
		textbackend->inputmethod.deathcount = 0;
	}

	if (++textbackend->inputmethod.deathcount > 5) {
		nemolog_error("INPUTMETHOD", "give up to launch input method\n");
		return;
	}

	nemolog_warning("INPUTMETHOD", "respawning input method...\n");

	textbackend_launch_input_method(textbackend);
}

static void textbackend_launch_input_method(struct textbackend *textbackend)
{
	if (textbackend->inputmethod.binding != NULL)
		return;

	if (textbackend->inputmethod.path == NULL)
		return;

	if (textbackend->inputmethod.task.pid != 0)
		return;

	nemolog_message("INPUTMETHOD", "try to launch input method (%s)\n", textbackend->inputmethod.path);

	textbackend->inputmethod.client = nemotask_launch(textbackend->compz,
			&textbackend->inputmethod.task,
			textbackend->inputmethod.path,
			textbackend_cleanup_input_method);
	if (textbackend->inputmethod.client == NULL)
		nemolog_error("INPUTMETHOD", "failed to launch input method client (%s)\n", textbackend->inputmethod.path);
}

static void textbackend_handle_compz_destroy(struct wl_listener *listener, void *data)
{
	struct textbackend *textbackend = (struct textbackend *)container_of(listener, struct textbackend, destroy_listener);

	if (textbackend->inputmethod.client != NULL)
		wl_client_destroy(textbackend->inputmethod.client);

	if (textbackend->inputmethod.path != NULL)
		free(textbackend->inputmethod.path);

	wl_list_remove(&textbackend->destroy_listener.link);

	free(textbackend);
}

struct textbackend *textbackend_create(struct nemocompz *compz, const char *inputpath)
{
	struct textbackend *textbackend;

	textbackend = (struct textbackend *)malloc(sizeof(struct textbackend));
	if (textbackend == NULL)
		return NULL;
	memset(textbackend, 0, sizeof(struct textbackend));

	textbackend->compz = compz;

	textbackend->destroy_listener.notify = textbackend_handle_compz_destroy;
	wl_signal_add(&compz->destroy_signal, &textbackend->destroy_listener);

	if (inputpath != NULL) {
		textbackend->inputmethod.path = strdup(inputpath);
	} else {
		textbackend->inputmethod.path = NULL;
	}

	inputmethod_create(compz->seat, textbackend);

	textbackend_launch_input_method(textbackend);

	textinput_create_manager(compz);

	return textbackend;
}
