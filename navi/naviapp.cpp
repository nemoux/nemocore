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

#include <naviapp.hpp>
#include <nemomisc.h>

NaviApp::NaviApp()
{
}

void NaviApp::OnBeforeCommandLineProcessing(const CefString &process_type, CefRefPtr<CefCommandLine> command_line)
{
	command_line->AppendSwitch("off-screen-rendering-enabled");
	command_line->AppendSwitch("disable-surfaces");
	command_line->AppendSwitch("disable-gpu");
	command_line->AppendSwitch("disable-gpu-compositing");
	command_line->AppendSwitch("enable-begin-frame-scheduling");
}

void NaviApp::OnContextInitialized()
{
}

void NaviApp::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line)
{
}

void NaviApp::OnRenderProcessThreadCreated(CefRefPtr<CefListValue> extra_info)
{
}
