#ifndef	__NEMOUX_PDF_HELPER_H__
#define	__NEMOUX_PDF_HELPER_H__

#include <stdint.h>

#include <pixman.h>
#include <mupdf/fitz.h>

struct nemopdf {
	fz_context *ctx;
	fz_document *doc;
	int32_t pagecount;
	int32_t pageindex;

	int32_t width, height;
};

extern struct nemopdf *nemopdf_create_doc(const char *filepath);
extern void nemopdf_destroy_doc(struct nemopdf *pdf);

extern int nemopdf_get_page_size(struct nemopdf *pdf, int ipage, float sx, float sy);
extern int nemopdf_render_page(struct nemopdf *pdf, int ipage, pixman_image_t *dst);

static inline int32_t nemopdf_get_page_count(struct nemopdf *pdf)
{
	return pdf->pagecount;
}

static inline int32_t nemopdf_get_page_index(struct nemopdf *pdf)
{
	return pdf->pageindex;
}

static inline int32_t nemopdf_get_page_width(struct nemopdf *pdf)
{
	return pdf->width;
}

static inline int32_t nemopdf_get_page_height(struct nemopdf *pdf)
{
	return pdf->height;
}

#endif
