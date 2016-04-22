#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemonavi.h>
#include <naviapp.hpp>
#include <navihandler.hpp>
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

	CefRefPtr<NaviApp> app(new NaviApp);
	CefInitialize(args, settings, app.get(), NULL);
	CefRunMessageLoop();
	CefShutdown();

	return 0;
}
