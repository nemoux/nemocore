#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <string>
#include <sstream>

#include <cef/cef_app.h>
#include <cef/cef_browser.h>
#include <cef/cef_command_line.h>
#include <cef/base/cef_bind.h>
#include <cef/wrapper/cef_closure_task.h>
#include <cef/wrapper/cef_helpers.h>

#include <nemonavi.h>
#include <navihandler.hpp>
#include <nemolog.h>
#include <nemomisc.h>

NaviHandler *g_instance = NULL;

NaviHandler::NaviHandler() : is_closing(false)
{
	g_instance = this;
}

NaviHandler::~NaviHandler()
{
	g_instance = NULL;
}

NaviHandler *NaviHandler::GetInstance()
{
	return g_instance;
}

void NaviHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString &title)
{
}

void NaviHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
	CEF_REQUIRE_UI_THREAD();

	browsers.push_back(browser);
	nemolog_message("NAVI", "...\n");
}

bool NaviHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
	CEF_REQUIRE_UI_THREAD();

	nemolog_message("NAVI", "...\n");
	if (browsers.size() == 1) {
		is_closing = true;
	}

	return false;
}

void NaviHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
	CEF_REQUIRE_UI_THREAD();
	nemolog_message("NAVI", "...\n");

	BrowserList::iterator iter = browsers.begin();

	for (; iter != browsers.end(); iter++) {
		if ((*iter)->IsSame(browser)) {
			browsers.erase(iter);
			break;
		}
	}

	if (browsers.empty()) {
		CefQuitMessageLoop();
	}
}

void NaviHandler::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl)
{
	CEF_REQUIRE_UI_THREAD();

	nemolog_message("NAVI", "...\n");
	if (errorCode == ERR_ABORTED)
		return;

	std::stringstream ss;

	ss << "<html><body bgcolor=\"white\">"
		"<h2>Failed to load URL " << std::string(failedUrl) <<
		" with error " << std::string(errorText) << " (" << errorCode <<
		").</h2></body></html>";

	frame->LoadString(ss.str(), failedUrl);
}

bool NaviHandler::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	nemolog_message("NAVI", "...\n");
}

bool NaviHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	nemolog_message("NAVI", "...\n");
}

bool NaviHandler::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int &screenX, int &screenY)
{
	nemolog_message("NAVI", "...\n");
}

bool NaviHandler::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screen_info)
{
	nemolog_message("NAVI", "...\n");
}

void NaviHandler::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
	nemolog_message("NAVI", "...\n");
}

void NaviHandler::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect &rect)
{
	nemolog_message("NAVI", "...\n");
}

void NaviHandler::OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList &dirtyRects, const void *buffer, int width, int height)
{
	nemolog_message("NAVI", "...\n");
}

void NaviHandler::OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor, CursorType type, const CefCursorInfo &custom_cursor_info)
{
}

void NaviHandler::OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, const CefString &suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback)
{
}

void NaviHandler::OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, CefRefPtr<CefDownloadItemCallback> callback)
{
}

bool NaviHandler::StartDragging(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDragData> drag_data, CefRenderHandler::DragOperationsMask allowed_ops, int x, int y)
{
}

void NaviHandler::UpdateDragCursor(CefRefPtr<CefBrowser> browser, CefRenderHandler::DragOperation operation)
{
}

void NaviHandler::CloseAllBrowsers(bool force_close)
{
	if (!CefCurrentlyOn(TID_UI)) {
		CefPostTask(TID_UI, base::Bind(&NaviHandler::CloseAllBrowsers, this, force_close));
		return;
	}

	if (browsers.empty())
		return;

	BrowserList::const_iterator iter = browsers.begin();

	for (; iter != browsers.end(); iter++) {
		(*iter)->GetHost()->CloseBrowser(force_close);
	}
}
