#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cef/cef_app.h>
#include <cef/cef_client.h>
#include <cef/cef_render_handler.h>

#include <nemonavi.h>
#include <nemonavi.hpp>
#include <naviclient.hpp>

int nemonavi_init_once(int argc, char *argv[])
{
	CefMainArgs args(argc, argv);

	if (CefExecuteProcess(args, NULL, NULL) >= 0)
		return -1;

	CefSettings settings;
	CefInitialize(args, settings, NULL, NULL);

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
