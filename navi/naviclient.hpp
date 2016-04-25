#ifndef __NEMONAVI_CLIENT_HPP__
#define __NEMONAVI_CLIENT_HPP__

#include <cef/cef_client.h>
#include <cef/cef_browser.h>
#include <cef/cef_request_handler.h>
#include <cef/cef_life_span_handler.h>
#include <cef/cef_drag_handler.h>
#include <cef/cef_geolocation_handler.h>
#include <cef/cef_keyboard_handler.h>

class NaviClient : public CefClient, public CefContextMenuHandler, public CefDisplayHandler, public CefDownloadHandler, public CefDragHandler, public CefGeolocationHandler, public CefLifeSpanHandler, public CefLoadHandler, public CefRenderHandler, public CefKeyboardHandler, public CefRequestHandler {
	public:
		NaviClient(void *data);
		~NaviClient();

		CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() OVERRIDE
		{
			return this;
		}

		CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE
		{
			return this;
		}

		CefRefPtr<CefDownloadHandler> GetDownloadHandler() OVERRIDE
		{
			return this;
		}

		CefRefPtr<CefDragHandler> GetDragHandler() OVERRIDE
		{
			return this;
		}

		CefRefPtr<CefGeolocationHandler> GetGeolocationHandler() OVERRIDE
		{
			return this;
		}

		CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE
		{
			return this;
		}

		CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE
		{
			return this;
		}

		CefRefPtr<CefRenderHandler> GetRenderHandler() OVERRIDE
		{
			return this;
		}

		CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() OVERRIDE
		{
			return this;
		}

		CefRefPtr<CefRequestHandler> GetRequestHandler() OVERRIDE
		{
			return this;
		}

		void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) OVERRIDE;
		bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, int command_id, EventFlags event_flags) OVERRIDE;

		void OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &url) OVERRIDE;
		void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString &title) OVERRIDE;
		void OnFullscreenModeChange(CefRefPtr<CefBrowser> browser, bool fullscreen) OVERRIDE;
		bool OnConsoleMessage(CefRefPtr<CefBrowser> browser, const CefString &message, const CefString &source, int line) OVERRIDE;

		bool OnBeforePopup(
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
				bool *no_javascript_access) OVERRIDE;
		void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
		bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
		void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

		void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) OVERRIDE;
		void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) OVERRIDE;

		bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool is_redirect) OVERRIDE;
		bool OnOpenURLFromTab(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &target_url, CefRequestHandler::WindowOpenDisposition target_disposition, bool user_gesture) OVERRIDE;
		cef_return_value_t OnBeforeResourceLoad(
				CefRefPtr<CefBrowser> browser,
				CefRefPtr<CefFrame> frame,
				CefRefPtr<CefRequest> request,
				CefRefPtr<CefRequestCallback> callback) OVERRIDE;
		CefRefPtr<CefResourceHandler> GetResourceHandler(
				CefRefPtr<CefBrowser> browser,
				CefRefPtr<CefFrame> frame,
				CefRefPtr<CefRequest> request) OVERRIDE;
		CefRefPtr<CefResponseFilter> GetResourceResponseFilter(
				CefRefPtr<CefBrowser> browser,
				CefRefPtr<CefFrame> frame,
				CefRefPtr<CefRequest> request,
				CefRefPtr<CefResponse> response) OVERRIDE;
		bool OnQuotaRequest(CefRefPtr<CefBrowser> browser, const CefString &origin_url, int64 new_size, CefRefPtr<CefRequestCallback> callback) OVERRIDE;
		void OnProtocolExecution(CefRefPtr<CefBrowser> browser, const CefString &url, bool &allow_os_execution) OVERRIDE;
		bool OnCertificateError(
				CefRefPtr<CefBrowser> browser,
				ErrorCode cert_error,
				const CefString &request_url,
				CefRefPtr<CefSSLInfo> ssl_info,
				CefRefPtr<CefRequestCallback> callback) OVERRIDE;
		void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status) OVERRIDE;

		bool GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect) OVERRIDE;
		bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) OVERRIDE;
		bool GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int &screenX, int &screenY) OVERRIDE;
		bool GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screen_info) OVERRIDE;

		void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) OVERRIDE;
		void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect &rect) OVERRIDE;

		void OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList &dirtyRects, const void *buffer, int width, int height) OVERRIDE;

		void OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor, CursorType type, const CefCursorInfo &custom_cursor_info) OVERRIDE;

		void OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, const CefString &suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback) OVERRIDE;
		void OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, CefRefPtr<CefDownloadItemCallback> callback) OVERRIDE;

		bool OnDragEnter(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDragData> dragData, CefDragHandler::DragOperationsMask mask) OVERRIDE;
		void OnDraggableRegionsChanged(CefRefPtr<CefBrowser> browser, const std::vector<CefDraggableRegion> &regions) OVERRIDE;

		bool OnRequestGeolocationPermission(
				CefRefPtr<CefBrowser> browser,
				const CefString &requesting_url,
				int request_id,
				CefRefPtr<CefGeolocationCallback> callback) OVERRIDE;

		bool OnPreKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent &event, CefEventHandle os_event, bool *is_keyboard_shortcut) OVERRIDE;

	private:
		IMPLEMENT_REFCOUNTING(NaviClient);
		DISALLOW_COPY_AND_ASSIGN(NaviClient);

		void *m_userdata;
};

#endif
