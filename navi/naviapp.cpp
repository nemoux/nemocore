#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cef/cef_browser.h>
#include <cef/cef_command_line.h>
#include <cef/wrapper/cef_helpers.h>
#include <cef/cef_cookie.h>

#include <nemonavi.h>
#include <naviapp.hpp>
#include <navihandler.hpp>
#include <nemolog.h>
#include <nemomisc.h>

NaviApp::NaviApp()
{
}

void NaviApp::OnBeforeCommandLineProcessing(const CefString &process_type, CefRefPtr<CefCommandLine> command_line)
{
	if (process_type.empty()) {
		if (command_line->HasSwitch("off-screen-rendering-enabled")) {
			if (!command_line->HasSwitch("disable-extensions") &&
					!command_line->HasSwitch("disable-pdf-extension")) {
				command_line->AppendSwitch("disable-surfaces");
			}

			if (!command_line->HasSwitch("enable-gpu")) {
				command_line->AppendSwitch("disable-gpu");
				command_line->AppendSwitch("disable-gpu-compositing");
			}

			command_line->AppendSwitch("enable-begin-frame-scheduling");
		}
	}
}

void NaviApp::OnContextInitialized()
{
	CEF_REQUIRE_UI_THREAD();

	CefWindowInfo window_info;
	window_info.SetAsWindowless(0x1, true);

	CefRefPtr<NaviHandler> handler(new NaviHandler());

	CefBrowserSettings browser_settings;
	std::string url;

	CefRefPtr<CefCommandLine> command_line = CefCommandLine::GetGlobalCommandLine();

	url = command_line->GetSwitchValue("url");
	if (url.empty())
		url = "http://www.google.com";

	CefBrowserHost::CreateBrowser(window_info, handler.get(), url, browser_settings, NULL);
}

void NaviApp::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line)
{
}

void NaviApp::OnRenderProcessThreadCreated(CefRefPtr<CefListValue> extra_info)
{
}
