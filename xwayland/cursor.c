#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <X11/Xcursor/Xcursor.h>
#include <linux/input.h>
#include <wayland-server.h>

#include <compz.h>
#include <shell.h>
#include <seat.h>
#include <xserver.h>
#include <xmanager.h>
#include <cursor.h>
#include <nemomisc.h>

static const char *bottom_left_corners[] = {
	"bottom_left_corner",
	"sw-resize",
	"size_bdiag"
};

static const char *bottom_right_corners[] = {
	"bottom_right_corner",
	"se-resize",
	"size_fdiag"
};

static const char *bottom_sides[] = {
	"bottom_side",
	"s-resize",
	"size_ver"
};

static const char *left_ptrs[] = {
	"left_ptr",
	"default",
	"top_left_arrow",
	"left-arrow"
};

static const char *left_sides[] = {
	"left_side",
	"w-resize",
	"size_hor"
};

static const char *right_sides[] = {
	"right_side",
	"e-resize",
	"size_hor"
};

static const char *top_left_corners[] = {
	"top_left_corner",
	"nw-resize",
	"size_fdiag"
};

static const char *top_right_corners[] = {
	"top_right_corner",
	"ne-resize",
	"size_bdiag"
};

static const char *top_sides[] = {
	"top_side",
	"n-resize",
	"size_ver"
};

struct cursor_alternatives {
	const char **names;
	size_t count;
};

static const struct cursor_alternatives cursors[] = {
	{ top_sides, ARRAY_LENGTH(top_sides) },
	{ bottom_sides, ARRAY_LENGTH(bottom_sides) },
	{ left_sides, ARRAY_LENGTH(left_sides) },
	{ right_sides, ARRAY_LENGTH(right_sides) },
	{ top_left_corners, ARRAY_LENGTH(top_left_corners) },
	{ top_right_corners, ARRAY_LENGTH(top_right_corners) },
	{ bottom_left_corners, ARRAY_LENGTH(bottom_left_corners) },
	{ bottom_right_corners, ARRAY_LENGTH(bottom_right_corners) },
	{ left_ptrs, ARRAY_LENGTH(left_ptrs) },
};

static xcb_cursor_t xcb_cursor_image_load_cursor(struct nemoxmanager *xmanager, const XcursorImage *img)
{
	xcb_connection_t *c = xmanager->conn;
	xcb_screen_iterator_t s = xcb_setup_roots_iterator(xcb_get_setup(c));
	xcb_screen_t *screen = s.data;
	xcb_gcontext_t gc;
	xcb_pixmap_t pix;
	xcb_render_picture_t pic;
	xcb_cursor_t cursor;
	int stride = img->width * 4;

	pix = xcb_generate_id(c);
	xcb_create_pixmap(c, 32, pix, screen->root, img->width, img->height);

	pic = xcb_generate_id(c);
	xcb_render_create_picture(c, pic, pix, xmanager->format_rgba.id, 0, 0);

	gc = xcb_generate_id(c);
	xcb_create_gc(c, gc, pix, 0, 0);

	xcb_put_image(c, XCB_IMAGE_FORMAT_Z_PIXMAP, pix, gc,
			img->width, img->height, 0, 0, 0, 32,
			stride * img->height, (uint8_t *)img->pixels);
	xcb_free_gc(c, gc);

	cursor = xcb_generate_id(c);
	xcb_render_create_cursor(c, cursor, pic, img->xhot, img->yhot);

	xcb_render_free_picture(c, pic);
	xcb_free_pixmap(c, pix);

	return cursor;
}

static xcb_cursor_t xcb_cursor_images_load_cursor(struct nemoxmanager *xmanager, const XcursorImages *images)
{
	if (images->nimage != 1)
		return -1;

	return xcb_cursor_image_load_cursor(xmanager, images->images[0]);
}

static xcb_cursor_t xcb_cursor_library_load_cursor(struct nemoxmanager *xmanager, const char *file)
{
	xcb_cursor_t cursor;
	XcursorImages *images;
	int size;

	if (!file)
		return 0;

	size = env_get_integer("XCURSOR_SIZE", 32);

	images = XcursorLibraryLoadImages(file, NULL, size);
	if (!images)
		return -1;

	cursor = xcb_cursor_images_load_cursor(xmanager, images);
	XcursorImagesDestroy(images);

	return cursor;
}

int nemoxmanager_create_cursors(struct nemoxmanager *xmanager)
{
	const char *name;
	int i, j, count = ARRAY_LENGTH(cursors);

	xmanager->cursors = malloc(sizeof(xcb_cursor_t) * count);
	if (xmanager->cursors == NULL)
		return -1;
	memset(xmanager->cursors, 0, sizeof(xcb_cursor_t) * count);

	for (i = 0; i < count; i++) {
		for (j = 0; j < cursors[i].count; j++) {
			name = cursors[i].names[j];
			xmanager->cursors[i] = xcb_cursor_library_load_cursor(xmanager, name);
			if (xmanager->cursors[i] != (xcb_cursor_t)-1)
				break;
		}
	}

	xmanager->last_cursor = -1;
}

void nemoxmanager_destroy_cursors(struct nemoxmanager *xmanager)
{
	int i;

	for (i = 0; i < ARRAY_LENGTH(cursors); i++)
		xcb_free_cursor(xmanager->conn, xmanager->cursors[i]);

	free(xmanager->cursors);
}

void nemoxmanager_set_cursor(struct nemoxmanager *xmanager, xcb_window_t wid, int cursor)
{
	uint32_t cursor_value_list;

	if (xmanager->last_cursor == cursor)
		return;

	xmanager->last_cursor = cursor;

	cursor_value_list = xmanager->cursors[cursor];

	xcb_change_window_attributes(xmanager->conn, wid, XCB_CW_CURSOR, &cursor_value_list);
	xcb_flush(xmanager->conn);
}
