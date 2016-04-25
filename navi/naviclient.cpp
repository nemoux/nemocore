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

NaviClient::NaviClient(void *data) : m_userdata(data)
{
}

NaviClient::~NaviClient()
{
}

void NaviClient::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model)
{
}

bool NaviClient::OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, int command_id, EventFlags event_flags)
{
	return true;
}

void NaviClient::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &url)
{
	CEF_REQUIRE_UI_THREAD();
}

void NaviClient::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString &title)
{
	CEF_REQUIRE_UI_THREAD();
}

void NaviClient::OnFullscreenModeChange(CefRefPtr<CefBrowser> browser, bool fullscreen)
{
	CEF_REQUIRE_UI_THREAD();
}

bool NaviClient::OnConsoleMessage(CefRefPtr<CefBrowser> browser, const CefString &message, const CefString &source, int line)
{
	CEF_REQUIRE_UI_THREAD();

	return false;
}

bool NaviClient::OnBeforePopup(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		const CefString &target_url,
		const CefString &target_frame_name,
		CefLifeSpanHandler::WindowOpenDisposition target_disposition,
		bool user_gesture,
		const CefPopupFeatures &popup_features,
		CefWindowInfo &window_info,
		CefRefPtr<CefClient> &client,
		CefBrowserSettings &settings,
		bool *no_javascript_access)
{
	CefRefPtr<CefRequest> request(CefRequest::Create());

	request->SetURL(target_url);

	frame->LoadRequest(request);

	return true;
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

bool NaviClient::OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool is_redirect)
{
	return false;
}

bool NaviClient::OnOpenURLFromTab(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &target_url, CefRequestHandler::WindowOpenDisposition target_disposition, bool user_gesture)
{
	return false;
}

bool NaviClient::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	return true;
}

bool NaviClient::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	struct nemonavi *navi = (struct nemonavi *)m_userdata;

	rect = CefRect(0, 0, navi->width, navi->height);

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
	struct nemonavi *navi = (struct nemonavi *)m_userdata;

	if (navi->dispatch_popup_show != NULL)
		navi->dispatch_popup_show(navi, show == true);
}

void NaviClient::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect &rect)
{
	struct nemonavi *navi = (struct nemonavi *)m_userdata;

	if (navi->dispatch_popup_rect != NULL)
		navi->dispatch_popup_rect(navi,
				rect.x, rect.y, rect.width, rect.height);
}

void NaviClient::OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList &dirtyRects, const void *buffer, int width, int height)
{
	struct nemonavi *navi = (struct nemonavi *)m_userdata;

	if (navi->dispatch_paint != NULL) {
		CefRenderHandler::RectList::const_iterator iter = dirtyRects.begin();

		for (; iter != dirtyRects.end(); iter++) {
			const CefRect &rect = *iter;

			navi->dispatch_paint(navi, type == PET_VIEW ? NEMONAVI_PAINT_VIEW_TYPE : NEMONAVI_PAINT_POPUP_TYPE, buffer, width, height, rect.x, rect.y, rect.width, rect.height);
		}
	}
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
