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
#include <naviclient.hpp>
#include <nemoshow.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

int main(int argc, char *argv[])
{
	CefMainArgs args(argc, argv);
	int r;

	r = CefExecuteProcess(args, NULL, NULL);
	if (r >= 0)
		return r;

	CefSettings settings;
	CefInitialize(args, settings, NULL, NULL);

	CefRefPtr<CefBrowser> browser;
	CefRefPtr<NaviClient> client;
	CefWindowInfo window_info;
	CefBrowserSettings browser_settings;

	window_info.SetAsWindowless(0x1, true);

	client = new NaviClient();

	browser = CefBrowserHost::CreateBrowserSync(window_info, client.get(), "http://www.google.com", browser_settings, NULL);

	while (1)
		CefDoMessageLoopWork();

	CefShutdown();

	return 0;
}
