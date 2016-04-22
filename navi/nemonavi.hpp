#ifndef __NEMONAVI_HPP__
#define __NEMONAVI_HPP__

#include <naviclient.hpp>

typedef struct _nemonavi {
	CefRefPtr<CefBrowser> browser;
	CefRefPtr<NaviClient> client;
} nemonavi_t;

#define NEMONAVI_CC(base, name)			(((nemonavi_t *)((base)->cc))->name)

#endif
