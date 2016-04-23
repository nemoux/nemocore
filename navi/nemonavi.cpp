#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <linux/input.h>

#include <cef/cef_app.h>
#include <cef/cef_client.h>
#include <cef/cef_request.h>
#include <cef/cef_render_handler.h>

#include <nemonavi.h>
#include <nemonavi.hpp>
#include <naviapp.hpp>
#include <naviclient.hpp>
#include <nemolog.h>
#include <nemomisc.h>

int nemonavi_init_once(int argc, char *argv[])
{
	CefMainArgs args(argc, argv);
	CefRefPtr<NaviApp> app = new NaviApp();

	if (CefExecuteProcess(args, app, NULL) >= 0)
		return -1;

	CefSettings settings;
	CefInitialize(args, settings, app, NULL);

	return 0;
}

void nemonavi_exit_once(void)
{
	CefShutdown();
}

void nemonavi_loop_once(void)
{
	CefDoMessageLoopWork();
}

struct nemonavi *nemonavi_create(const char *url)
{
	struct nemonavi *navi;

	navi = (struct nemonavi *)malloc(sizeof(struct nemonavi));
	if (navi == NULL)
		return NULL;
	memset(navi, 0, sizeof(struct nemonavi));

	CefWindowInfo window_info;
	window_info.SetAsWindowless((uint64_t)navi, true);

	CefBrowserSettings browser_settings;

	navi->cc = new nemonavi_t;
	NEMONAVI_CC(navi, client) = new NaviClient(navi);
	NEMONAVI_CC(navi, browser) = CefBrowserHost::CreateBrowserSync(
			window_info,
			NEMONAVI_CC(navi, client).get(),
			url,
			browser_settings,
			NULL);

	return navi;
}

void nemonavi_destroy(struct nemonavi *navi)
{
	delete static_cast<nemonavi_t *>(navi->cc);

	free(navi);
}

void nemonavi_set_size(struct nemonavi *navi, int32_t width, int32_t height)
{
	navi->width = width;
	navi->height = height;

	NEMONAVI_CC(navi, browser)->GetHost()->WasResized();
}

void nemonavi_send_pointer_enter_event(struct nemonavi *navi, float x, float y)
{
	NEMONAVI_CC(navi, browser)->GetHost()->SendFocusEvent(true);
}

void nemonavi_send_pointer_leave_event(struct nemonavi *navi, float x, float y)
{
	CefMouseEvent mouse_event;

	mouse_event.x = x;
	mouse_event.y = y;
	mouse_event.modifiers = 0x0;

	NEMONAVI_CC(navi, browser)->GetHost()->SendMouseMoveEvent(mouse_event, true);
	NEMONAVI_CC(navi, browser)->GetHost()->SendFocusEvent(false);
}

void nemonavi_send_pointer_down_event(struct nemonavi *navi, float x, float y, int button)
{
	CefBrowserHost::MouseButtonType button_type;
	CefMouseEvent mouse_event;

	if (button == BTN_LEFT)
		button_type = MBT_LEFT;
	else if (button == BTN_RIGHT)
		button_type = MBT_RIGHT;
	else if (button == BTN_MIDDLE)
		button_type = MBT_MIDDLE;

	mouse_event.x = x;
	mouse_event.y = y;
	mouse_event.modifiers = 0x0;

	NEMONAVI_CC(navi, browser)->GetHost()->SendMouseClickEvent(mouse_event, button_type, false, 1);
}

void nemonavi_send_pointer_up_event(struct nemonavi *navi, float x, float y, int button)
{
	CefBrowserHost::MouseButtonType button_type;
	CefMouseEvent mouse_event;

	if (button == BTN_LEFT)
		button_type = MBT_LEFT;
	else if (button == BTN_RIGHT)
		button_type = MBT_RIGHT;
	else if (button == BTN_MIDDLE)
		button_type = MBT_MIDDLE;

	mouse_event.x = x;
	mouse_event.y = y;
	mouse_event.modifiers = 0x0;

	NEMONAVI_CC(navi, browser)->GetHost()->SendMouseClickEvent(mouse_event, button_type, true, 1);
}

void nemonavi_send_pointer_motion_event(struct nemonavi *navi, float x, float y)
{
	CefMouseEvent mouse_event;

	mouse_event.x = x;
	mouse_event.y = y;
	mouse_event.modifiers = 0x0;

	NEMONAVI_CC(navi, browser)->GetHost()->SendMouseMoveEvent(mouse_event, false);
}

void nemonavi_send_keyboard_down_event(struct nemonavi *navi, uint32_t code)
{
	CefKeyEvent key_event;

	key_event.windows_key_code = code;
	key_event.native_key_code = code;
	key_event.character = code;
	key_event.modifiers = 0x0;

	key_event.type = KEYEVENT_RAWKEYDOWN;
	NEMONAVI_CC(navi, browser)->GetHost()->SendKeyEvent(key_event);
}

void nemonavi_send_keyboard_up_event(struct nemonavi *navi, uint32_t code)
{
	CefKeyEvent key_event;

	key_event.windows_key_code = code;
	key_event.native_key_code = code;
	key_event.character = code;
	key_event.modifiers = 0x0;

	key_event.type = KEYEVENT_KEYUP;
	NEMONAVI_CC(navi, browser)->GetHost()->SendKeyEvent(key_event);

	key_event.type = KEYEVENT_CHAR;
	NEMONAVI_CC(navi, browser)->GetHost()->SendKeyEvent(key_event);
}

void nemonavi_send_touch_down_event(struct nemonavi *navi, float x, float y, uint32_t id)
{
}

void nemonavi_send_touch_up_event(struct nemonavi *navi, float x, float y, uint32_t id)
{
}

void nemonavi_send_touch_motion_event(struct nemonavi *navi, float x, float y, uint32_t id)
{
}

void nemonavi_load_url(struct nemonavi *navi, const char *url)
{
	CefRefPtr<CefRequest> request(CefRequest::Create());

	request->SetURL(url);

	NEMONAVI_CC(navi, browser)->GetMainFrame()->LoadRequest(request);
}
