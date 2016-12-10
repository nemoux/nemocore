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
#include <naviapp.hpp>
#include <nemolog.h>
#include <nemomisc.h>

NaviClient::NaviClient(void *data) : m_userdata(data)
{
}

NaviClient::~NaviClient()
{
}

void NaviClient::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model)
{
	model->Clear();

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif
}

bool NaviClient::OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, int command_id, EventFlags event_flags)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	return true;
}

void NaviClient::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &url)
{
	CEF_REQUIRE_UI_THREAD();

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d] url(%s)\n", __FUNCTION__, __LINE__, std::string(url).c_str());
#endif
}

void NaviClient::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString &title)
{
	CEF_REQUIRE_UI_THREAD();

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d] title(%s)\n", __FUNCTION__, __LINE__, std::string(title).c_str());
#endif
}

void NaviClient::OnFullscreenModeChange(CefRefPtr<CefBrowser> browser, bool fullscreen)
{
	CEF_REQUIRE_UI_THREAD();

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif
}

bool NaviClient::OnConsoleMessage(CefRefPtr<CefBrowser> browser, const CefString &message, const CefString &source, int line)
{
	CEF_REQUIRE_UI_THREAD();

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d] source(%s:%d) message(%s)\n", __FUNCTION__, __LINE__, std::string(source).c_str(), line, std::string(message).c_str());
#endif

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

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	request->SetURL(target_url);

	frame->LoadRequest(request);

	return true;
}

void NaviClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
	CEF_REQUIRE_UI_THREAD();

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif
}

bool NaviClient::DoClose(CefRefPtr<CefBrowser> browser)
{
	CEF_REQUIRE_UI_THREAD();

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	return false;
}

void NaviClient::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
	CEF_REQUIRE_UI_THREAD();

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif
}

void NaviClient::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward)
{
	struct nemonavi *navi = (struct nemonavi *)m_userdata;

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	if (navi->dispatch_loading_state != NULL)
		navi->dispatch_loading_state(navi, isLoading == true, canGoBack == true, canGoForward == true);
}

void NaviClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl)
{
	CEF_REQUIRE_UI_THREAD();

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d] URL(%s) error(%s)\n", __FUNCTION__, __LINE__, std::string(failedUrl).c_str(), std::string(errorText).c_str());
#endif

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
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	return false;
}

bool NaviClient::OnOpenURLFromTab(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &target_url, CefRequestHandler::WindowOpenDisposition target_disposition, bool user_gesture)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d] target_url(%s)\n", __FUNCTION__, __LINE__, std::string(target_url).c_str());
#endif

	return false;
}

cef_return_value_t NaviClient::OnBeforeResourceLoad(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefRequest> request,
		CefRefPtr<CefRequestCallback> callback)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	return RV_CONTINUE;
}

CefRefPtr<CefResourceHandler> NaviClient::GetResourceHandler(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefRequest> request)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	return NULL;
}

CefRefPtr<CefResponseFilter> NaviClient::GetResourceResponseFilter(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefRequest> request,
		CefRefPtr<CefResponse> response)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	return NULL;
}

bool NaviClient::OnQuotaRequest(CefRefPtr<CefBrowser> browser, const CefString &origin_url, int64 new_size, CefRefPtr<CefRequestCallback> callback)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d] origin_url(%s)\n", __FUNCTION__, __LINE__, std::string(origin_url).c_str());
#endif

	return true;
}

void NaviClient::OnProtocolExecution(CefRefPtr<CefBrowser> browser, const CefString &url, bool &allow_os_execution)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d] url(%s) allow(%d)\n", __FUNCTION__, __LINE__, std::string(url).c_str(), allow_os_execution);
#endif
}

bool NaviClient::OnCertificateError(
		CefRefPtr<CefBrowser> browser,
		ErrorCode cert_error,
		const CefString &request_url,
		CefRefPtr<CefSSLInfo> ssl_info,
		CefRefPtr<CefRequestCallback> callback)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d] request_url(%s)\n", __FUNCTION__, __LINE__, std::string(request_url).c_str());
#endif

	return false;
}

void NaviClient::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif
}

bool NaviClient::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	return true;
}

bool NaviClient::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	struct nemonavi *navi = (struct nemonavi *)m_userdata;

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	rect = CefRect(0, 0, navi->width, navi->height);

	return true;
}

bool NaviClient::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int &screenX, int &screenY)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	return true;
}

bool NaviClient::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screen_info)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	return true;
}

void NaviClient::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
	struct nemonavi *navi = (struct nemonavi *)m_userdata;

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	if (navi->dispatch_popup_show != NULL)
		navi->dispatch_popup_show(navi, show == true);
}

void NaviClient::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect &rect)
{
	struct nemonavi *navi = (struct nemonavi *)m_userdata;

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

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
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif
}

void NaviClient::OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, const CefString &suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d] suggested_name(%s)\n", __FUNCTION__, __LINE__, std::string(suggested_name).c_str());
#endif
}

void NaviClient::OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, CefRefPtr<CefDownloadItemCallback> callback)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif
}

bool NaviClient::OnDragEnter(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDragData> dragData, CefDragHandler::DragOperationsMask mask)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	return false;
}

void NaviClient::OnDraggableRegionsChanged(CefRefPtr<CefBrowser> browser, const std::vector<CefDraggableRegion> &regions)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif
}

bool NaviClient::OnRequestGeolocationPermission(
		CefRefPtr<CefBrowser> browser,
		const CefString &requesting_url,
		int request_id,
		CefRefPtr<CefGeolocationCallback> callback)
{
#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	callback->Continue(true);

	return true;
}

bool NaviClient::OnPreKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent &event, CefEventHandle os_event, bool *is_keyboard_shortcut)
{
	struct nemonavi *navi = (struct nemonavi *)m_userdata;

#ifdef NEMONAVI_DEBUG_ON
	nemolog_message("NAVI", "[%s:%d]\n", __FUNCTION__, __LINE__);
#endif

	if (navi->dispatch_key_event != NULL)
		navi->dispatch_key_event(navi, event.windows_key_code, event.focus_on_editable_field);

	return false;
}

bool NaviClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{
	struct nemonavi *navi = (struct nemonavi *)m_userdata;
	std::string name = message->GetName();

	if (name == NEMONAVI_FOCUSED_NODE_CHANGED_MESSAGE) {
		if (navi->dispatch_focus_change != NULL)
			navi->dispatch_focus_change(navi, message->GetArgumentList()->GetBool(0));

		return true;
	}

	return false;
}
