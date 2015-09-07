#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <png.h>
#include <jpeglib.h>

#include <pixmanhelper.h>

static void a8r8g8b8_to_rgba_np(uint8_t *dst, uint8_t *src, int width, int height, int stride)
{
	int i, j;

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			uint8_t a, r, g, b, t;

			a = src[i * stride + j * 4 + 3];
			r = src[i * stride + j * 4 + 2];
			g = src[i * stride + j * 4 + 1];
			b = src[i * stride + j * 4 + 0];

			if (a != 0) {
#define DIVIDE(c, a, t)		t = ((c) * 255) / a; (c) = t < 0 ? 0 : t > 255 ? 255 : t;
				DIVIDE(r, a, t);
				DIVIDE(g, a, t);
				DIVIDE(b, a, t);
			}

			dst[i * stride + j * 4 + 3] = a;
			dst[i * stride + j * 4 + 2] = b;
			dst[i * stride + j * 4 + 1] = g;
			dst[i * stride + j * 4 + 0] = r;
		}
	}
}

int pixman_save_png_file(pixman_image_t *image, const char *path)
{
	int width = pixman_image_get_width(image);
	int height = pixman_image_get_height(image);
	int stride = pixman_image_get_stride(image);
	uint8_t *data;
	pixman_image_t *copy;
	png_struct *write_struct;
	png_info *info_struct;
	pixman_bool_t result = 0;
	FILE *f;
	png_bytep *row_pointers;
	int i;

	f = fopen(path, "wb");
	if (f == NULL)
		return 0;

	data = (uint8_t *)malloc(sizeof(uint8_t) * height * stride);
	if (data == NULL)
		goto out1;

	row_pointers = (png_bytep *)malloc(height * sizeof(png_bytep));
	if (row_pointers == NULL)
		goto out2;

	copy = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, (uint32_t *)data, stride);

	pixman_image_composite32(PIXMAN_OP_SRC, image, NULL, copy, 0, 0, 0, 0, 0, 0, width, height);

	a8r8g8b8_to_rgba_np(data, data, width, height, stride);

	for (i = 0; i < height; ++i)
		row_pointers[i] = (png_bytep)(data + i * stride);

	if (!(write_struct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
		goto out3;

	if (!(info_struct = png_create_info_struct(write_struct)))
		goto out4;

	png_init_io(write_struct, f);

	png_set_IHDR(write_struct, info_struct, width, height,
			8, PNG_COLOR_TYPE_RGB_ALPHA,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
			PNG_FILTER_TYPE_BASE);

	png_write_info(write_struct, info_struct);

	png_write_image(write_struct, row_pointers);

	png_write_end(write_struct, NULL);

	result = 1;

out4:
	png_destroy_write_struct(&write_struct, &info_struct);

out3:
	pixman_image_unref(copy);
	free(row_pointers);

out2:
	free(data);

out1:
	fclose(f);

	return result;
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
		goto out;

	pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (pngptr == NULL)
		goto out;

	infoptr = png_create_info_struct(pngptr);
	if (infoptr == NULL)
		goto out;

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

	if (colortype == PNG_COLOR_TYPE_GRAY ||
			colortype == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(pngptr);

	png_read_update_info(pngptr, infoptr);

	rowpointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
	for (i = 0; i < height; i++)
		rowpointers[i] = (png_byte *)malloc(png_get_rowbytes(pngptr, infoptr));

	png_read_image(pngptr, rowpointers);

	image = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, NULL, width * 4);
	if (image == NULL)
		goto out;

	data = pixman_image_get_data(image);

	for (i = 0; i < height; i++) {
		src = (uint8_t *)rowpointers[i];
		dst = (uint8_t *)&data[i * width];

		for (j = 0; j < width; j++) {
			dst[j * 4 + 2] = src[j * 4 + 0];
			dst[j * 4 + 1] = src[j * 4 + 1];
			dst[j * 4 + 0] = src[j * 4 + 2];
			dst[j * 4 + 3] = src[j * 4 + 3];
		}
	}

	for (i = 0; i < height; i++)
		free(rowpointers[i]);
	free(rowpointers);

out:
	fclose(fp);

	return image;
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

	image = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, NULL, width * 4);
	if (image == NULL)
		goto out;

	data = pixman_image_get_data(image);

	while (cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, buffer, 1);

		src = (uint8_t *)buffer[0];
		dst = (uint8_t *)&data[(cinfo.output_scanline - 1) * cinfo.output_width];

		for (i = 0; i < cinfo.output_width; i++) {
			dst[i * 4 + 2] = src[i * 3 + 0];
			dst[i * 4 + 1] = src[i * 3 + 1];
			dst[i * 4 + 0] = src[i * 3 + 2];
			dst[i * 4 + 3] = 255;
		}
	}

	jpeg_finish_decompress(&cinfo);

	jpeg_destroy_decompress(&cinfo);

out:
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
		goto out;

	infoptr = png_create_info_struct(pngptr);
	if (infoptr == NULL)
		goto out;

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

	if (colortype == PNG_COLOR_TYPE_GRAY ||
			colortype == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(pngptr);

	png_read_update_info(pngptr, infoptr);

	rowpointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
	for (i = 0; i < height; i++)
		rowpointers[i] = (png_byte *)malloc(png_get_rowbytes(pngptr, infoptr));

	png_read_image(pngptr, rowpointers);

	image = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, NULL, width * 4);
	if (image == NULL)
		goto out;

	data = pixman_image_get_data(image);

	for (i = 0; i < height; i++) {
		src = (char *)rowpointers[i];
		dst = (char *)&data[i * width];

		for (j = 0; j < width; j++) {
			dst[j * 4 + 2] = src[j * 4 + 0];
			dst[j * 4 + 1] = src[j * 4 + 1];
			dst[j * 4 + 0] = src[j * 4 + 2];
			dst[j * 4 + 3] = src[j * 4 + 3];
		}
	}

	for (i = 0; i < height; i++)
		free(rowpointers[i]);
	free(rowpointers);

	png_destroy_info_struct(pngptr, &infoptr);

	png_destroy_read_struct(&pngptr, &infoptr, &infoptr);

out:
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

	image = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, NULL, width * 4);
	if (image == NULL)
		goto out;

	data = pixman_image_get_data(image);

	while (cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, buffer, 1);

		src = (char *)buffer[0];
		dst = (char *)&data[(cinfo.output_scanline - 1) * cinfo.output_width];

		for (i = 0; i < cinfo.output_width; i++) {
			dst[i * 4 + 2] = src[i * 3 + 0];
			dst[i * 4 + 1] = src[i * 3 + 1];
			dst[i * 4 + 0] = src[i * 3 + 2];
			dst[i * 4 + 3] = 255;
		}
	}

	jpeg_finish_decompress(&cinfo);

	jpeg_destroy_decompress(&cinfo);

out:
	return image;
}

pixman_image_t *pixman_load_image(const char *filepath, int32_t width, int32_t height)
{
	pixman_image_t *src;
	pixman_image_t *dst;
	pixman_transform_t transform;

	src = pixman_load_png_file(filepath);
	if (src == NULL)
		src = pixman_load_jpeg_file(filepath);
	if (src == NULL)
		return NULL;

	if (pixman_image_get_width(src) == width &&
			pixman_image_get_height(src) == height)
		return src;

	dst = pixman_image_create_bits(PIXMAN_a8r8g8b8,
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
