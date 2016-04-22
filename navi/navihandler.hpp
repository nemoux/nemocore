#ifndef __NEMONAVI_HANDLER_HPP__
#define __NEMONAVI_HANDLER_HPP__

#include <list>

#include <cef/cef_client.h>
#include <cef/cef_browser.h>

class NaviHandler : public CefClient, public CefContextMenuHandler, public CefDisplayHandler, public CefDownloadHandler, public CefLifeSpanHandler, public CefLoadHandler, public CefRenderHandler, public CefKeyboardHandler, public CefRequestHandler {
	public:
		NaviHandler();
		~NaviHandler();

		static NaviHandler *GetInstance();

		CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE
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

		void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString &title) OVERRIDE;
		void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
		bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
		void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

		void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) OVERRIDE;

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

		bool StartDragging(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDragData> drag_data, CefRenderHandler::DragOperationsMask allowed_ops, int x, int y) OVERRIDE;
		void UpdateDragCursor(CefRefPtr<CefBrowser> browser, CefRenderHandler::DragOperation operation) OVERRIDE;

		void CloseAllBrowsers(bool force_close);

		bool IsClosing() const
		{
			return is_closing;
		}

	private:
		typedef std::list< CefRefPtr<CefBrowser> > BrowserList;
		BrowserList browsers;

		bool is_closing;

		IMPLEMENT_REFCOUNTING(NaviHandler);
		DISALLOW_COPY_AND_ASSIGN(NaviHandler);
};

#endif
