#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <png.h>
#include <jpeglib.h>
#include <cairo.h>
#include <librsvg/rsvg.h>

#include <pixmanhelper.h>
#include <nemomisc.h>

int pixman_save_png_file(pixman_image_t *image, const char *path)
{
	int width = pixman_image_get_width(image);
	int height = pixman_image_get_height(image);
	uint8_t *data;
	pixman_image_t *copy;
	pixman_bool_t r = 0;
	png_structp pngptr;
	png_infop infoptr;
	png_bytep *rowpointers;
	FILE *fp;
	int i;

	fp = fopen(path, "wb");
	if (fp == NULL)
		return 0;

	data = (uint8_t *)malloc(sizeof(uint8_t[4]) * width * height);
	if (data == NULL)
		goto out1;

	rowpointers = (png_bytep *)malloc(height * sizeof(png_bytep));
	if (rowpointers == NULL)
		goto out2;

	copy = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, width, height, (uint32_t *)data, width * 4);

	pixman_image_composite32(PIXMAN_OP_SRC, image, NULL, copy, 0, 0, 0, 0, 0, 0, width, height);

	for (i = 0; i < height; ++i)
		rowpointers[i] = (png_bytep)(data + i * width * 4);

	if (!(pngptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
		goto out3;

	if (!(infoptr = png_create_info_struct(pngptr)))
		goto out4;

	png_init_io(pngptr, fp);

	png_set_IHDR(pngptr, infoptr, width, height,
			8, PNG_COLOR_TYPE_RGB_ALPHA,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
			PNG_FILTER_TYPE_BASE);

	png_write_info(pngptr, infoptr);

	png_write_image(pngptr, rowpointers);

	png_write_end(pngptr, NULL);

	png_destroy_info_struct(pngptr, &infoptr);

	r = 1;

out4:
	png_destroy_write_struct(&pngptr, &infoptr);

out3:
	pixman_image_unref(copy);
	free(rowpointers);

out2:
	free(data);

out1:
	fclose(fp);

	return r;
}

pixman_image_t *pixman_load_png_file(const char *path)
{
	pixman_image_t *image = NULL;
	png_structp pngptr;
	png_infop infoptr;
	FILE *fp;
	int width, height;
	png_byte colortype;
	png_byte bitdepth;
	png_bytep *rowpointers;
	unsigned char header[8];
	uint32_t *data;
	uint8_t *src, *dst;
	int i, j;

	fp = fopen(path, "rb");
	if (fp == NULL)
		return NULL;

	fread(header, 1, 8, fp);

	if (png_sig_cmp(header, 0, 8))
		goto out1;

	pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (pngptr == NULL)
		goto out1;

	infoptr = png_create_info_struct(pngptr);
	if (infoptr == NULL)
		goto out2;

	png_init_io(pngptr, fp);
	png_set_sig_bytes(pngptr, 8);

	png_read_info(pngptr, infoptr);

	width = png_get_image_width(pngptr, infoptr);
	height = png_get_image_height(pngptr, infoptr);
	colortype = png_get_color_type(pngptr, infoptr);
	bitdepth = png_get_bit_depth(pngptr, infoptr);

	if (bitdepth == 16)
		png_set_strip_16(pngptr);

	if (colortype == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(pngptr);

	if (colortype == PNG_COLOR_TYPE_GRAY && bitdepth < 8)
		png_set_expand_gray_1_2_4_to_8(pngptr);

	if (png_get_valid(pngptr, infoptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(pngptr);

	if (colortype == PNG_COLOR_TYPE_RGB ||
			colortype == PNG_COLOR_TYPE_GRAY ||
			colortype == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(pngptr, 0xff, PNG_FILLER_AFTER);

	if (colortype == PNG_COLOR_TYPE_RGB ||
			colortype == PNG_COLOR_TYPE_RGB_ALPHA)
		png_set_bgr(pngptr);

	if (colortype == PNG_COLOR_TYPE_GRAY ||
			colortype == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(pngptr);

	png_read_update_info(pngptr, infoptr);

	image = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, width, height, NULL, width * 4);
	if (image == NULL)
		goto out3;
	data = pixman_image_get_data(image);

	rowpointers = (png_bytep *)malloc(sizeof(png_bytep) * height);

	for (i = 0; i < height; i++) {
		rowpointers[i] = (png_bytep)&data[i * width];
	}

	png_read_image(pngptr, rowpointers);

	free(rowpointers);

out3:
	png_destroy_info_struct(pngptr, &infoptr);

out2:
	png_destroy_read_struct(&pngptr, &infoptr, &infoptr);

out1:
	fclose(fp);

	return image;
}

struct pngbuffer {
	uint8_t *buffer;
	int length;
	int offset;
};

static void pixman_read_callback_for_png(png_structp pngptr, png_bytep out, png_size_t count)
{
	struct pngbuffer *buffer = (struct pngbuffer *)png_get_io_ptr(pngptr);

	memcpy(out, buffer->buffer + buffer->offset, count);

	buffer->offset += count;
}

pixman_image_t *pixman_load_png_data(uint32_t *data, int length)
{
	pixman_image_t *image = NULL;
	png_structp pngptr;
	png_infop infoptr;
	int width, height;
	png_byte colortype;
	png_byte bitdepth;
	png_bytep *rowpointers;
	struct pngbuffer buffer;
	char *src, *dst;
	int i, j;

	buffer.buffer = (uint8_t *)data;
	buffer.length = length;
	buffer.offset = 8;

	if (png_sig_cmp(buffer.buffer, 0, 8))
		return NULL;

	pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (pngptr == NULL)
		goto out1;

	infoptr = png_create_info_struct(pngptr);
	if (infoptr == NULL)
		goto out2;

	png_set_read_fn(pngptr, (void *)&buffer, pixman_read_callback_for_png);

	png_set_sig_bytes(pngptr, 8);

	png_read_info(pngptr, infoptr);

	width = png_get_image_width(pngptr, infoptr);
	height = png_get_image_height(pngptr, infoptr);
	colortype = png_get_color_type(pngptr, infoptr);
	bitdepth = png_get_bit_depth(pngptr, infoptr);

	if (bitdepth == 16)
		png_set_strip_16(pngptr);

	if (colortype == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(pngptr);

	if (colortype == PNG_COLOR_TYPE_GRAY && bitdepth < 8)
		png_set_expand_gray_1_2_4_to_8(pngptr);

	if (png_get_valid(pngptr, infoptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(pngptr);

	if (colortype == PNG_COLOR_TYPE_RGB ||
			colortype == PNG_COLOR_TYPE_GRAY ||
			colortype == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(pngptr, 0xff, PNG_FILLER_AFTER);

	if (colortype == PNG_COLOR_TYPE_RGB ||
			colortype == PNG_COLOR_TYPE_RGB_ALPHA)
		png_set_bgr(pngptr);

	if (colortype == PNG_COLOR_TYPE_GRAY ||
			colortype == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(pngptr);

	png_read_update_info(pngptr, infoptr);

	image = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, width, height, NULL, width * 4);
	if (image == NULL)
		goto out3;
	data = pixman_image_get_data(image);

	rowpointers = (png_bytep *)malloc(sizeof(png_bytep) * height);

	for (i = 0; i < height; i++) {
		rowpointers[i] = (png_bytep)&data[i * width];
	}

	png_read_image(pngptr, rowpointers);

	free(rowpointers);

out3:
	png_destroy_info_struct(pngptr, &infoptr);

out2:
	png_destroy_read_struct(&pngptr, &infoptr, &infoptr);

out1:
	return image;
}

struct jpegerror {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

static void jpegerror_exit(j_common_ptr cinfo)
{
	struct jpegerror *err = (struct jpegerror *)cinfo->err;

	(*cinfo->err->output_message)(cinfo);

	longjmp(err->setjmp_buffer, 1);
}

pixman_image_t *pixman_load_jpeg_file(const char *path)
{
	pixman_image_t *image = NULL;
	struct jpeg_decompress_struct cinfo;
	JSAMPARRAY buffer;
	struct jpegerror jerr;
	FILE *fp;
	int width, height;
	uint32_t *data;
	uint8_t *src, *dst;
	int rowstride;
	int i;

	fp = fopen(path, "rb");
	if (fp == NULL)
		return NULL;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = jpegerror_exit;

	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		goto out;
	}

	jpeg_create_decompress(&cinfo);

	jpeg_stdio_src(&cinfo, fp);

	jpeg_read_header(&cinfo, TRUE);

	jpeg_start_decompress(&cinfo);

	rowstride = cinfo.output_width * cinfo.output_components;

	buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, rowstride, 1);

	width = cinfo.output_width;
	height = cinfo.output_height;

	image = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, width, height, NULL, width * 4);
	if (image == NULL)
		goto out;

	data = pixman_image_get_data(image);

	while (cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, buffer, 1);

		src = (uint8_t *)buffer[0];
		dst = (uint8_t *)&data[(cinfo.output_scanline - 1) * cinfo.output_width];

		for (i = 0; i < cinfo.output_width; i++) {
			dst[i * 4 + 0] = src[i * 3 + 0];
			dst[i * 4 + 1] = src[i * 3 + 1];
			dst[i * 4 + 2] = src[i * 3 + 2];
			dst[i * 4 + 3] = 255;
		}
	}

	jpeg_finish_decompress(&cinfo);

	jpeg_destroy_decompress(&cinfo);

out:
	fclose(fp);

	return image;
}

pixman_image_t *pixman_load_jpeg_data(uint32_t *data, int length)
{
	pixman_image_t *image = NULL;
	struct jpeg_decompress_struct cinfo;
	JSAMPARRAY buffer;
	struct jpegerror jerr;
	int width, height;
	char *src, *dst;
	int rowstride;
	int i;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = jpegerror_exit;

	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		goto out;
	}

	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, (unsigned char *)data, length);

	jpeg_read_header(&cinfo, TRUE);

	jpeg_start_decompress(&cinfo);

	rowstride = cinfo.output_width * cinfo.output_components;

	buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, rowstride, 1);

	width = cinfo.output_width;
	height = cinfo.output_height;

	image = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, width, height, NULL, width * 4);
	if (image == NULL)
		goto out;

	data = pixman_image_get_data(image);

	while (cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, buffer, 1);

		src = (char *)buffer[0];
		dst = (char *)&data[(cinfo.output_scanline - 1) * cinfo.output_width];

		for (i = 0; i < cinfo.output_width; i++) {
			dst[i * 4 + 0] = src[i * 3 + 0];
			dst[i * 4 + 1] = src[i * 3 + 1];
			dst[i * 4 + 2] = src[i * 3 + 2];
			dst[i * 4 + 3] = 255;
		}
	}

	jpeg_finish_decompress(&cinfo);

	jpeg_destroy_decompress(&cinfo);

out:
	return image;
}

pixman_image_t *pixman_load_svg_file(const char *path)
{
	pixman_image_t *image = NULL;
	RsvgHandle *rsvg;
	RsvgDimensionData dims;
	cairo_surface_t *surface;
	cairo_t *cr;

	rsvg = rsvg_handle_new_from_file(path, NULL);
	if (rsvg == NULL)
		return NULL;

	rsvg_handle_get_dimensions(rsvg, &dims);

	image = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, dims.width, dims.height, NULL, dims.width * 4);
	if (image == NULL)
		goto out1;

	surface = cairo_image_surface_create_for_data(
			(unsigned char *)pixman_image_get_data(image),
			CAIRO_FORMAT_ARGB32,
			pixman_image_get_width(image),
			pixman_image_get_height(image),
			pixman_image_get_stride(image));
	cr = cairo_create(surface);
	rsvg_handle_render_cairo(rsvg, cr);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);

out1:
	g_object_unref(rsvg);

	return image;
}

pixman_image_t *pixman_load_svg_data(uint32_t *data, int length)
{
	pixman_image_t *image = NULL;
	RsvgHandle *rsvg;
	RsvgDimensionData dims;
	cairo_surface_t *surface;
	cairo_t *cr;

	rsvg = rsvg_handle_new_from_data((unsigned char *)data, length, NULL);
	if (rsvg == NULL)
		return NULL;

	rsvg_handle_get_dimensions(rsvg, &dims);

	image = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, dims.width, dims.height, NULL, dims.width * 4);
	if (image == NULL)
		goto out1;

	surface = cairo_image_surface_create_for_data(
			(unsigned char *)pixman_image_get_data(image),
			CAIRO_FORMAT_ARGB32,
			pixman_image_get_width(image),
			pixman_image_get_height(image),
			pixman_image_get_stride(image));
	cr = cairo_create(surface);
	rsvg_handle_render_cairo(rsvg, cr);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);

out1:
	g_object_unref(rsvg);

	return image;
}

static pixman_image_t *pixman_load_jpeg_image(const char *filepath, int32_t width, int32_t height)
{
	return pixman_load_jpeg_file(filepath);
}

static pixman_image_t *pixman_load_png_image(const char *filepath, int32_t width, int32_t height)
{
	return pixman_load_png_file(filepath);
}

static pixman_image_t *pixman_load_svg_image(const char *filepath, int32_t width, int32_t height)
{
	pixman_image_t *image = NULL;
	RsvgHandle *rsvg;
	RsvgDimensionData dims;
	cairo_surface_t *surface;
	cairo_t *cr;

	rsvg = rsvg_handle_new_from_file(filepath, NULL);
	if (rsvg == NULL)
		return NULL;

	rsvg_handle_get_dimensions(rsvg, &dims);

	image = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, width, height, NULL, width * 4);
	if (image == NULL)
		goto out1;

	surface = cairo_image_surface_create_for_data(
			(unsigned char *)pixman_image_get_data(image),
			CAIRO_FORMAT_ARGB32,
			pixman_image_get_width(image),
			pixman_image_get_height(image),
			pixman_image_get_stride(image));
	cr = cairo_create(surface);
	cairo_scale(cr, (double)width / (double)dims.width, (double)height / (double)dims.height);
	rsvg_handle_render_cairo(rsvg, cr);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);

out1:
	g_object_unref(rsvg);

	return image;
}

static int pixman_compare_extension(const void *a, const void *b)
{
	return strcasecmp((const char *)a, (const char *)b);
}

pixman_image_t *pixman_load_image(const char *filepath, int32_t width, int32_t height)
{
	static struct extensionelement {
		char extension[32];

		pixman_image_t *(*load)(const char *path, int32_t width, int32_t height);
	} elements[] = {
		{ ".jpeg",			pixman_load_jpeg_image },
		{ ".jpg",				pixman_load_jpeg_image },
		{ ".png",				pixman_load_png_image },
		{ ".svg",				pixman_load_svg_image }
	}, *element;

	pixman_image_t *src;
	const char *extension = strrchr(filepath, '.');

	if (extension == NULL)
		return NULL;

	element = (struct extensionelement *)bsearch(extension, elements, sizeof(elements) / sizeof(elements[0]), sizeof(elements[0]), pixman_compare_extension);
	if (element != NULL && (src = element->load(filepath, width, height)) != NULL) {
		pixman_image_t *dst;
		pixman_transform_t transform;

		if ((width == 0 && height == 0) || (pixman_image_get_width(src) == width && pixman_image_get_height(src) == height))
			return src;

		if (width == 0)
			width = pixman_image_get_width(src);
		if (height == 0)
			height = pixman_image_get_height(src);

		dst = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8,
				width, height,
				NULL,
				width * 4);

		pixman_transform_init_identity(&transform);
		pixman_transform_scale(&transform, NULL,
				pixman_double_to_fixed(
					(double)pixman_image_get_width(src) / (double)pixman_image_get_width(dst)),
				pixman_double_to_fixed(
					(double)pixman_image_get_height(src) / (double)pixman_image_get_height(dst)));

		pixman_image_set_transform(src, &transform);

		pixman_image_composite32(PIXMAN_OP_SRC,
				src,
				NULL,
				dst,
				0, 0, 0, 0, 0, 0,
				width, height);

		pixman_image_unref(src);

		return dst;
	}

	return NULL;
}

pixman_image_t *pixman_resize_image(pixman_image_t *src, int32_t width, int32_t height)
{
	pixman_image_t *dst;
	pixman_transform_t transform;

	dst = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8,
			width, height,
			NULL,
			width * 4);

	pixman_transform_init_identity(&transform);
	pixman_transform_scale(&transform, NULL,
			pixman_double_to_fixed(
				(double)pixman_image_get_width(src) / (double)pixman_image_get_width(dst)),
			pixman_double_to_fixed(
				(double)pixman_image_get_height(src) / (double)pixman_image_get_height(dst)));

	pixman_image_set_transform(src, &transform);

	pixman_image_composite32(PIXMAN_OP_SRC,
			src,
			NULL,
			dst,
			0, 0, 0, 0, 0, 0,
			width, height);

	return dst;
}

pixman_image_t *pixman_clone_image(pixman_image_t *src)
{
	pixman_image_t *dst;

	dst = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8,
			pixman_image_get_width(src),
			pixman_image_get_height(src),
			NULL,
			pixman_image_get_stride(src));

	pixman_image_composite32(PIXMAN_OP_SRC,
			src,
			NULL,
			dst,
			0, 0, 0, 0, 0, 0,
			pixman_image_get_width(src),
			pixman_image_get_height(src));

	return dst;
}

int pixman_copy_image(pixman_image_t *dst, pixman_image_t *src)
{
	pixman_transform_t transform;

	pixman_transform_init_identity(&transform);
	pixman_transform_scale(&transform, NULL,
			pixman_double_to_fixed(
				(double)pixman_image_get_width(src) / (double)pixman_image_get_width(dst)),
			pixman_double_to_fixed(
				(double)pixman_image_get_height(src) / (double)pixman_image_get_height(dst)));

	pixman_image_set_transform(src, &transform);

	pixman_image_composite32(PIXMAN_OP_SRC,
			src,
			NULL,
			dst,
			0, 0, 0, 0, 0, 0,
			pixman_image_get_width(dst),
			pixman_image_get_height(dst));

	return 0;
}
