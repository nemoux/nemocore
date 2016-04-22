#ifndef __NEMONAVI_APP_HPP__
#define __NEMONAVI_APP_HPP__

#include <cef/cef_app.h>

class NaviApp : public CefApp, public CefBrowserProcessHandler {
	public:
		NaviApp();

		CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() OVERRIDE
		{
			return this;
		}

		void OnBeforeCommandLineProcessing(const CefString &process_type, CefRefPtr<CefCommandLine> command_line) OVERRIDE;

		void OnContextInitialized() OVERRIDE;
		void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) OVERRIDE;
		void OnRenderProcessThreadCreated(CefRefPtr<CefListValue> extra_info) OVERRIDE;

	private:
		IMPLEMENT_REFCOUNTING(NaviApp);
		DISALLOW_COPY_AND_ASSIGN(NaviApp);
};

#endif
