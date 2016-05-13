#ifndef __NEMONAVI_APP_HPP__
#define __NEMONAVI_APP_HPP__

#include <cef/cef_app.h>

class NaviApp : public CefApp, public CefBrowserProcessHandler, public CefRenderProcessHandler {
	public:
		NaviApp();

		CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() OVERRIDE
		{
			return this;
		}

		CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() OVERRIDE
		{
			return this;
		}

		void OnBeforeCommandLineProcessing(const CefString &process_type, CefRefPtr<CefCommandLine> command_line) OVERRIDE;

		void OnContextInitialized() OVERRIDE;
		void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) OVERRIDE;
		void OnRenderProcessThreadCreated(CefRefPtr<CefListValue> extra_info) OVERRIDE;

		void OnRenderThreadCreated(CefRefPtr<CefListValue> extra_info) OVERRIDE;
		void OnWebKitInitialized() OVERRIDE;
		void OnBrowserCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
		void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) OVERRIDE;
		CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE;
		bool OnBeforeNavigation(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, NavigationType navigation_type, bool is_redirect) OVERRIDE;
		void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) OVERRIDE;
		void OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) OVERRIDE;
		void OnUncaughtException(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context, CefRefPtr<CefV8Exception> exception, CefRefPtr<CefV8StackTrace> stackTrace) OVERRIDE;
		void OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefDOMNode> node) OVERRIDE;
		bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) OVERRIDE;

	private:
		IMPLEMENT_REFCOUNTING(NaviApp);
		DISALLOW_COPY_AND_ASSIGN(NaviApp);
};

#endif
