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

void NaviApp::OnRenderThreadCreated(CefRefPtr<CefListValue> extra_info)
{
}

void NaviApp::OnWebKitInitialized()
{
}

void NaviApp::OnBrowserCreated(CefRefPtr<CefBrowser> browser)
{
}

void NaviApp::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser)
{
}

CefRefPtr<CefLoadHandler> NaviApp::GetLoadHandler()
{
	return NULL;
}

bool NaviApp::OnBeforeNavigation(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, NavigationType navigation_type, bool is_redirect)
{
	return false;
}

void NaviApp::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
}

void NaviApp::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
}

void NaviApp::OnUncaughtException(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context, CefRefPtr<CefV8Exception> exception, CefRefPtr<CefV8StackTrace> stackTrace)
{
}

void NaviApp::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefDOMNode> node)
{
}

bool NaviApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{
	return false;
}
