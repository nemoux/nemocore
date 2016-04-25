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
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
}

bool NaviClient::OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, int command_id, EventFlags event_flags)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return true;
}

void NaviClient::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &url)
{
	CEF_REQUIRE_UI_THREAD();

	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
}

void NaviClient::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString &title)
{
	CEF_REQUIRE_UI_THREAD();

	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
}

void NaviClient::OnFullscreenModeChange(CefRefPtr<CefBrowser> browser, bool fullscreen)
{
	CEF_REQUIRE_UI_THREAD();

	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
}

bool NaviClient::OnConsoleMessage(CefRefPtr<CefBrowser> browser, const CefString &message, const CefString &source, int line)
{
	CEF_REQUIRE_UI_THREAD();

	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

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

	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	request->SetURL(target_url);

	frame->LoadRequest(request);

	return true;
}

void NaviClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
	CEF_REQUIRE_UI_THREAD();

	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
}

bool NaviClient::DoClose(CefRefPtr<CefBrowser> browser)
{
	CEF_REQUIRE_UI_THREAD();

	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return false;
}

void NaviClient::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
	CEF_REQUIRE_UI_THREAD();

	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
}

void NaviClient::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
}

void NaviClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl)
{
	CEF_REQUIRE_UI_THREAD();

	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

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
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return false;
}

bool NaviClient::OnOpenURLFromTab(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &target_url, CefRequestHandler::WindowOpenDisposition target_disposition, bool user_gesture)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return false;
}

cef_return_value_t NaviClient::OnBeforeResourceLoad(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefRequest> request,
		CefRefPtr<CefRequestCallback> callback)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return RV_CONTINUE;
}

CefRefPtr<CefResourceHandler> NaviClient::GetResourceHandler(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefRequest> request)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return NULL;
}

CefRefPtr<CefResponseFilter> NaviClient::GetResourceResponseFilter(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefRequest> request,
		CefRefPtr<CefResponse> response)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return NULL;
}

bool NaviClient::OnQuotaRequest(CefRefPtr<CefBrowser> browser, const CefString &origin_url, int64 new_size, CefRefPtr<CefRequestCallback> callback)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return true;
}

void NaviClient::OnProtocolExecution(CefRefPtr<CefBrowser> browser, const CefString &url, bool &allow_os_execution)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
}

bool NaviClient::OnCertificateError(
		CefRefPtr<CefBrowser> browser,
		ErrorCode cert_error,
		const CefString &request_url,
		CefRefPtr<CefSSLInfo> ssl_info,
		CefRefPtr<CefRequestCallback> callback)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return false;
}

void NaviClient::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
}

bool NaviClient::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return true;
}

bool NaviClient::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	struct nemonavi *navi = (struct nemonavi *)m_userdata;

	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	rect = CefRect(0, 0, navi->width, navi->height);

	return true;
}

bool NaviClient::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int &screenX, int &screenY)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return true;
}

bool NaviClient::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screen_info)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return true;
}

void NaviClient::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
	struct nemonavi *navi = (struct nemonavi *)m_userdata;

	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	if (navi->dispatch_popup_show != NULL)
		navi->dispatch_popup_show(navi, show == true);
}

void NaviClient::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect &rect)
{
	struct nemonavi *navi = (struct nemonavi *)m_userdata;

	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	if (navi->dispatch_popup_rect != NULL)
		navi->dispatch_popup_rect(navi,
				rect.x, rect.y, rect.width, rect.height);
}

void NaviClient::OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList &dirtyRects, const void *buffer, int width, int height)
{
	struct nemonavi *navi = (struct nemonavi *)m_userdata;

	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

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
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
}

void NaviClient::OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, const CefString &suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
}

void NaviClient::OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, CefRefPtr<CefDownloadItemCallback> callback)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
}

bool NaviClient::OnDragEnter(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDragData> dragData, CefDragHandler::DragOperationsMask mask)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return false;
}

void NaviClient::OnDraggableRegionsChanged(CefRefPtr<CefBrowser> browser, const std::vector<CefDraggableRegion> &regions)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
}

bool NaviClient::OnRequestGeolocationPermission(
		CefRefPtr<CefBrowser> browser,
		const CefString &requesting_url,
		int request_id,
		CefRefPtr<CefGeolocationCallback> callback)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	callback->Continue(true);

	return true;
}

bool NaviClient::OnPreKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent &event, CefEventHandle os_event, bool *is_keyboard_shortcut)
{
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);

	return false;
}
