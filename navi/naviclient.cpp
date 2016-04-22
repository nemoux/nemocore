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
#include <naviclient.hpp>
#include <nemolog.h>
#include <nemohelper.h>
#include <nemomisc.h>

NaviClient::NaviClient()
{
}

NaviClient::~NaviClient()
{
}

void NaviClient::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString &title)
{
}

void NaviClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
	CEF_REQUIRE_UI_THREAD();
}

bool NaviClient::DoClose(CefRefPtr<CefBrowser> browser)
{
	CEF_REQUIRE_UI_THREAD();

	return false;
}

void NaviClient::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
	CEF_REQUIRE_UI_THREAD();
}

void NaviClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl)
{
	CEF_REQUIRE_UI_THREAD();

	if (errorCode == ERR_ABORTED)
		return;

	std::stringstream ss;

	ss << "<html><body bgcolor=\"white\">"
		"<h2>Failed to load URL " << std::string(failedUrl) <<
		" with error " << std::string(errorText) << " (" << errorCode <<
		").</h2></body></html>";

	frame->LoadString(ss.str(), failedUrl);
}

bool NaviClient::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	rect = CefRect(0, 0, 640, 480);

	return true;
}

bool NaviClient::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	rect = CefRect(0, 0, 640, 480);

	return true;
}

bool NaviClient::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int &screenX, int &screenY)
{
	return true;
}

bool NaviClient::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screen_info)
{
	return true;
}

void NaviClient::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
}

void NaviClient::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect &rect)
{
}

void NaviClient::OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList &dirtyRects, const void *buffer, int width, int height)
{
}

void NaviClient::OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor, CursorType type, const CefCursorInfo &custom_cursor_info)
{
}

void NaviClient::OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, const CefString &suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback)
{
}

void NaviClient::OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, CefRefPtr<CefDownloadItemCallback> callback)
{
}

bool NaviClient::StartDragging(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDragData> drag_data, CefRenderHandler::DragOperationsMask allowed_ops, int x, int y)
{
}

void NaviClient::UpdateDragCursor(CefRefPtr<CefBrowser> browser, CefRenderHandler::DragOperation operation)
{
}
