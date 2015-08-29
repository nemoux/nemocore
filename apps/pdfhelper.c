#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <pdfhelper.h>

struct nemopdf *nemopdf_create_doc(const char *filepath)
{
	struct nemopdf *pdf;

	pdf = (struct nemopdf *)malloc(sizeof(struct nemopdf));
	if (pdf == NULL)
		return NULL;
	memset(pdf, 0, sizeof(struct nemopdf));

	pdf->ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	if (pdf->ctx == NULL)
		goto err1;

	fz_register_document_handlers(pdf->ctx);

	pdf->doc = fz_open_document(pdf->ctx, filepath);
	if (pdf->doc == NULL)
		goto err2;

	pdf->pagecount = fz_count_pages(pdf->doc);

	nemopdf_get_page_size(pdf, 0, 1.0f, 1.0f);

	return pdf;

err2:
	fz_free_context(pdf->ctx);

err1:
	free(pdf);

	return NULL;
}

void nemopdf_destroy_doc(struct nemopdf *pdf)
{
	fz_close_document(pdf->doc);

	fz_free_context(pdf->ctx);

	free(pdf);
}

int nemopdf_get_page_size(struct nemopdf *pdf, int ipage, float sx, float sy)
{
	fz_page *page;
	fz_pixmap *pix;
	fz_matrix transform;
	fz_rect bounds;
	fz_irect bbox;

	page = fz_load_page(pdf->doc, ipage);
	if (page == NULL)
		return -1;

	fz_rotate(&transform, 0);
	fz_pre_scale(&transform, sx, sy);

	fz_bound_page(pdf->doc, page, &bounds);
	fz_transform_rect(&bounds, &transform);

	fz_round_rect(&bbox, &bounds);

	pix = fz_new_pixmap_with_bbox(pdf->ctx, fz_device_rgb(pdf->ctx), &bbox);
	fz_clear_pixmap_with_value(pdf->ctx, pix, 0xff);

	pdf->width = pix->w;
	pdf->height = pix->h;

	fz_drop_pixmap(pdf->ctx, pix);
	fz_free_page(pdf->doc, page);

	return 0;
}

int nemopdf_render_page(struct nemopdf *pdf, int ipage, pixman_image_t *dst)
{
	fz_page *page;
	fz_pixmap *pix;
	fz_device *dev;
	fz_matrix transform;
	fz_rect bounds;
	fz_irect bbox;
	pixman_image_t *src;
	int width = pixman_image_get_width(dst);
	int height = pixman_image_get_height(dst);
	float sx = (float)width / (float)pdf->width;
	float sy = (float)height / (float)pdf->height;

	page = fz_load_page(pdf->doc, ipage);
	if (page == NULL)
		return -1;

	fz_rotate(&transform, 0);
	fz_pre_scale(&transform, sx, sy);

	fz_bound_page(pdf->doc, page, &bounds);
	fz_transform_rect(&bounds, &transform);

	fz_round_rect(&bbox, &bounds);

	pix = fz_new_pixmap_with_bbox(pdf->ctx, fz_device_rgb(pdf->ctx), &bbox);
	fz_clear_pixmap_with_value(pdf->ctx, pix, 0xff);

	dev = fz_new_draw_device(pdf->ctx, pix);
	fz_run_page(pdf->doc, page, dev, &transform, NULL);
	fz_free_device(dev);

	src = pixman_image_create_bits(PIXMAN_a8b8g8r8, pix->w, pix->h, (uint32_t *)(pix->samples), pix->w * pix->n);
	pixman_image_composite32(PIXMAN_OP_SRC, src, NULL, dst, 0, 0, 0, 0, 0, 0, pix->w, pix->h);
	pixman_image_unref(src);

	fz_drop_pixmap(pdf->ctx, pix);
	fz_free_page(pdf->doc, page);

	pdf->pageindex = ipage;

	return 0;
}
