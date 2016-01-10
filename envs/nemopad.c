#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-nemo-shell-server-protocol.h>

#include <shell.h>
#include <compz.h>
#include <actor.h>
#include <layer.h>
#include <view.h>
#include <seat.h>
#include <touch.h>
#include <keypad.h>
#include <grab.h>
#include <move.h>
#include <pick.h>
#include <picker.h>
#include <timer.h>

#include <nemopad.h>
#include <nemograb.h>
#include <talehelper.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

static struct {
	const char *border;

	const char *normal;
	const char *upper;
	const char *shift;

	uint32_t x0, y0;
	uint32_t x1, y1;

	char name[16];
	uint32_t code;

	uint32_t color0;
	uint32_t color1;

	double thickness;
} nemopadkeys[NEMOPAD_KEYS_MAX] = {
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-base.svg", NULL, NULL, NULL, 0, 0, 964, 322, "BASE", 0, 2, 2, 1.0f },

	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-01.svg", NULL, NULL, 26, 4, 86, 30, "ESC", 1, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-02.svg", NULL, NULL, 76, 4, 136, 30, "FOCUS", 0, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-03.svg", NULL, NULL, 138, 4, 198, 30, "F1", 59, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-04.svg", NULL, NULL, 188, 4, 248, 30, "F2", 60, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-05.svg", NULL, NULL, 238, 4, 298, 30, "F3", 61, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-06.svg", NULL, NULL, 288, 4, 348, 30, "F4", 62, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-07.svg", NULL, NULL, 338, 4, 398, 30, "F5", 63, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-08.svg", NULL, NULL, 388, 4, 448, 30, "F6", 64, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-09.svg", NULL, NULL, 452, 4, 512, 30, "PICK", 0, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-10.svg", NULL, NULL, 516, 4, 576, 30, "F7", 65, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-11.svg", NULL, NULL, 566, 4, 626, 30, "F8", 66, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-12.svg", NULL, NULL, 616, 4, 676, 30, "F9", 67, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-13.svg", NULL, NULL, 666, 4, 726, 30, "F10", 68, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-14.svg", NULL, NULL, 716, 4, 776, 30, "F11", 87, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-15.svg", NULL, NULL, 766, 4, 826, 30, "F12", 88, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-16.svg", NULL, NULL, 828, 4, 888, 30, "FOCUS", 0, 2, 2, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-h.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-17.svg", NULL, NULL, 878, 4, 938, 30, "POWER", 0, 2, 2, 3.0f },

	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-18.svg", NULL, NEMOENVS_RESOURCES "/keyboard/keyboard-18-s.svg", 4, 42, 64, 94, "'", 41, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-19.svg", NULL, NEMOENVS_RESOURCES "/keyboard/keyboard-19-s.svg", 68, 42, 128, 94, "1", 2, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-20.svg", NULL, NEMOENVS_RESOURCES "/keyboard/keyboard-20-s.svg", 132, 42, 192, 94, "2", 3, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-21.svg", NULL, NEMOENVS_RESOURCES "/keyboard/keyboard-21-s.svg", 196, 42, 256, 94, "3", 4, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-22.svg", NULL, NEMOENVS_RESOURCES "/keyboard/keyboard-22-s.svg", 260, 42, 320, 94, "4", 5, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-23.svg", NULL, NEMOENVS_RESOURCES "/keyboard/keyboard-23-s.svg", 324, 42, 384, 94, "5", 6, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-24.svg", NULL, NEMOENVS_RESOURCES "/keyboard/keyboard-24-s.svg", 388, 42, 448, 94, "6", 7, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-25.svg", NULL, NEMOENVS_RESOURCES "/keyboard/keyboard-25-s.svg", 452, 42, 512, 94, "7", 8, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-26.svg", NULL, NEMOENVS_RESOURCES "/keyboard/keyboard-26-s.svg", 516, 42, 576, 94, "8", 9, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-27.svg", NULL, NEMOENVS_RESOURCES "/keyboard/keyboard-27-s.svg", 580, 42, 640, 94, "9", 10, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-28.svg", NULL, NEMOENVS_RESOURCES "/keyboard/keyboard-28-s.svg", 644, 42, 704, 94, "0", 11, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-29.svg", NULL, NEMOENVS_RESOURCES "/keyboard/keyboard-29-s.svg", 708, 42, 768, 94, "-", 12, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-30.svg", NULL, NEMOENVS_RESOURCES "/keyboard/keyboard-30-s.svg", 772, 42, 832, 94, "=", 13, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-d-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-31.svg", NULL, NULL, 821, 42, 960, 94, "BACK", 14, 1, 1, 3.0f },

	{ NEMOENVS_RESOURCES "/keyboard/keyboard-b.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-32.svg", NULL, NULL, 36, 98, 111, 150, "TAB", 15, 1, 1, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-33.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-33-s.svg", NULL, 100, 98, 160, 150, "q", 16, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-34.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-34-s.svg", NULL, 164, 98, 224, 150, "w", 17, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-35.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-35-s.svg", NULL, 228, 98, 288, 150, "e", 18, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-36.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-36-s.svg", NULL, 292, 98, 352, 150, "r", 19, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-37.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-37-s.svg", NULL, 356, 98, 416, 150, "t", 20, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-38.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-38-s.svg", NULL, 420, 98, 480, 150, "y", 21, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-39.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-39-s.svg", NULL, 484, 98, 544, 150, "u", 22, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-40.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-40-s.svg", NULL, 548, 98, 608, 150, "i", 23, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-41.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-41-s.svg", NULL, 612, 98, 672, 150, "o", 24, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-42.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-42-s.svg", NULL, 676, 98, 736, 150, "p", 25, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-43.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-43-s.svg", NULL, 740, 98, 800, 150, "[", 26, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-44.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-44-s.svg", NULL, 804, 98, 864, 150, "]", 27, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-45.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-45-s.svg", NULL, 868, 98, 928, 150, "\\", 43, 1, 2, 1.5f },

	{ NEMOENVS_RESOURCES "/keyboard/keyboard-c.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-46.svg", NULL, NULL, 36, 154, 143, 206, "CAPS", 58, 1, 1, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-47.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-47-s.svg", NULL, 132, 154, 192, 206, "a", 30, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-48.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-48-s.svg", NULL, 196, 154, 256, 206, "s", 31, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-49.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-49-s.svg", NULL, 260, 154, 320, 206, "d", 32, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-50.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-50-s.svg", NULL, 324, 154, 384, 206, "f", 33, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-51.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-51-s.svg", NULL, 388, 154, 448, 206, "g", 34, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-52.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-52-s.svg", NULL, 452, 154, 512, 206, "h", 35, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-53.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-53-s.svg", NULL, 516, 154, 576, 206, "j", 36, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-54.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-54-s.svg", NULL, 580, 154, 640, 206, "k", 37, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-55.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-55-s.svg", NULL, 644, 154, 704, 206, "l", 38, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-56.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-56-s.svg", NULL, 708, 154, 768, 206, ";", 39, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-57.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-57-s.svg", NULL, 772, 154, 832, 206, "'", 40, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-c-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-58.svg", NULL, NULL, 821, 154, 928, 206, "ENTER", 28, 1, 1, 3.0f },

	{ NEMOENVS_RESOURCES "/keyboard/keyboard-d.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-59.svg", NULL, NULL, 36, 210, 175, 262, "SHIFT", 42, 1, 1, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-60.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-60-s.svg", NULL, 164, 210, 224, 262, "z", 44, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-61.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-61-s.svg", NULL, 228, 210, 288, 262, "x", 45, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-62.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-62-s.svg", NULL, 292, 210, 352, 262, "c", 46, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-63.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-63-s.svg", NULL, 356, 210, 416, 262, "v", 47, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-64.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-64-s.svg", NULL, 420, 210, 480, 262, "b", 48, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-65.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-65-s.svg", NULL, 484, 210, 544, 262, "n", 49, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-66.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-66-s.svg", NULL, 548, 210, 608, 262, "m", 50, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-67.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-67-s.svg", NULL, 612, 210, 672, 262, ",", 51, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-68.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-68-s.svg", NULL, 676, 210, 736, 262, ".", 52, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-a.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-69.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-69-s.svg", NULL, 740, 210, 800, 262, "/", 53, 1, 2, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-d-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-70.svg", NULL, NULL, 789, 210, 928, 262, "SHIFT", 54, 1, 1, 3.0f },

	{ NEMOENVS_RESOURCES "/keyboard/keyboard-b.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-71.svg", NULL, NULL, 68, 266, 143, 318, "CTRL", 29, 1, 1, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-b.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-72.svg", NULL, NULL, 132, 266, 207, 318, "ALT", 56, 1, 1, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-e.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-73.svg", NULL, NULL, 181, 266, 655, 318, "SPACE", 57, 1, 1, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-b-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-74.svg", NULL, NULL, 629, 266, 704, 318, "CTRL", 29, 1, 1, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-b-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-75.svg", NULL, NULL, 693, 266, 768, 318, "ALT", 56, 1, 1, 3.0f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-f.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-76.svg", NULL, NULL, 772, 266, 802, 318, "LEFT", 75, 1, 1, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-g.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-77.svg", NULL, NULL, 805, 266, 863, 290, "UP", 72, 1, 1, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-g-i.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-78.svg", NULL, NULL, 805, 294, 863, 318, "DOWN", 80, 1, 1, 1.5f },
	{ NEMOENVS_RESOURCES "/keyboard/keyboard-f.svg", NEMOENVS_RESOURCES "/keyboard/keyboard-79.svg", NULL, NULL, 866, 266, 896, 318, "RIGHT", 77, 1, 1, 1.5f },
};

static struct showone *nemopadborders[NEMOPAD_KEYS_MAX];
static struct showone *nemopadnormals[NEMOPAD_KEYS_MAX];
static struct showone *nemopaduppers[NEMOPAD_KEYS_MAX];
static struct showone *nemopadshifts[NEMOPAD_KEYS_MAX];

static double nemopadcolors[][3] = {
	{ 0x0, 0x0, 0x0 },
	{ 0x1e, 0xdc, 0xdc },
	{ 0xff, 0x8c, 0x32 }
};

static struct showone *nemopadblur;
static struct showone *nemopadease;

void nemopad_prepare_envs(void)
{
	struct showone *one;
	int i;

	for (i = 0; i < NEMOPAD_KEYS_MAX; i++) {
		if (nemopadkeys[i].border != NULL) {
			nemopadborders[i] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one,
					nemopadkeys[i].x1 - nemopadkeys[i].x0);
			nemoshow_item_set_height(one,
					nemopadkeys[i].y1 - nemopadkeys[i].y0);
			nemoshow_item_load_svg(one, nemopadkeys[i].border);
		} else {
			nemopadborders[i] = NULL;
		}
		if (nemopadkeys[i].normal != NULL) {
			nemopadnormals[i] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one,
					nemopadkeys[i].x1 - nemopadkeys[i].x0);
			nemoshow_item_set_height(one,
					nemopadkeys[i].y1 - nemopadkeys[i].y0);
			nemoshow_item_load_svg(one, nemopadkeys[i].normal);
		} else {
			nemopadnormals[i] = NULL;
		}
		if (nemopadkeys[i].upper != NULL) {
			nemopaduppers[i] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one,
					nemopadkeys[i].x1 - nemopadkeys[i].x0);
			nemoshow_item_set_height(one,
					nemopadkeys[i].y1 - nemopadkeys[i].y0);
			nemoshow_item_load_svg(one, nemopadkeys[i].upper);
		} else {
			nemopaduppers[i] = NULL;
		}
		if (nemopadkeys[i].shift != NULL) {
			nemopadshifts[i] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one,
					nemopadkeys[i].x1 - nemopadkeys[i].x0);
			nemoshow_item_set_height(one,
					nemopadkeys[i].y1 - nemopadkeys[i].y0);
			nemoshow_item_load_svg(one, nemopadkeys[i].shift);
		} else {
			nemopadshifts[i] = NULL;
		}
	}

	nemopadblur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(nemopadblur, "high", "solid", 3.0f);

	nemopadease = nemoshow_ease_create();
	nemoshow_ease_set_type(nemopadease, NEMOEASE_CUBIC_INOUT_TYPE);
}

void nemopad_finish_envs(void)
{
	int i;

	for (i = 0; i < NEMOPAD_KEYS_MAX; i++) {
		if (nemopadborders[i] != NULL)
			nemoshow_one_destroy(nemopadborders[i]);
		if (nemopadnormals[i] != NULL)
			nemoshow_one_destroy(nemopadnormals[i]);
		if (nemopaduppers[i] != NULL)
			nemoshow_one_destroy(nemopaduppers[i]);
		if (nemopadshifts[i] != NULL)
			nemoshow_one_destroy(nemopadshifts[i]);
	}

	nemoshow_one_destroy(nemopadblur);
	nemoshow_one_destroy(nemopadease);
}

static int nemopad_dispatch_key_grab(struct talegrab *base, uint32_t type, struct taleevent *event)
{
	struct nemograb *grab = (struct nemograb *)container_of(base, struct nemograb, base);
	struct nemopad *pad = (struct nemopad *)nemograb_get_userdata(grab);
	uint32_t tag = nemograb_get_tag(grab);
	uint32_t code = nemopadkeys[tag].code;

	if (type & NEMOTALE_DOWN_EVENT) {
		struct nemoshow *show = pad->show;
		struct nemoshell *shell = NEMOSHOW_AT(show, shell);
		struct nemoactor *actor = NEMOSHOW_AT(show, actor);
		struct nemocompz *compz = shell->compz;
		struct nemoseat *seat = compz->seat;

		if (code != 0) {
			nemokeypad_notify_key(pad->keypad,
					time_current_msecs(),
					code,
					WL_KEYBOARD_KEY_STATE_PRESSED);
		}

		if (tag == 9) {
			struct touchpoint *tp;
			int i;

			for (i = 0; i < NEMOPAD_KEYS_MAX; i++) {
				if (pad->keys[i] != NULL) {
					nemoshow_item_set_fill_color(pad->keys[i], 0x0, 0x0, 0x0, 0x0);
				}
			}

			tp = nemoseat_get_touchpoint_by_id(seat, event->device);
			if (tp != NULL) {
				nemoshell_move_actor_by_touchpoint(shell, tp, actor);
			}

			pad->is_pickable = 1;
		}

		if (pad->borders[tag] != NULL) {
			if (pad->is_upper_case != 0 || pad->is_shift_case != 0) {
				nemoshow_item_set_fill_color(pad->borders[tag],
						nemopadcolors[nemopadkeys[tag].color1][0],
						nemopadcolors[nemopadkeys[tag].color1][1],
						nemopadcolors[nemopadkeys[tag].color1][2],
						0xc0);
			} else {
				nemoshow_item_set_fill_color(pad->borders[tag],
						nemopadcolors[nemopadkeys[tag].color0][0],
						nemopadcolors[nemopadkeys[tag].color0][1],
						nemopadcolors[nemopadkeys[tag].color0][2],
						0xc0);
			}
		}

		nemoactor_dispatch_frame(actor);
	} else if (type & NEMOTALE_UP_EVENT) {
		struct nemoshow *show = pad->show;
		struct nemoshell *shell = NEMOSHOW_AT(show, shell);
		struct nemoactor *actor = NEMOSHOW_AT(show, actor);

		if (code != 0) {
			nemokeypad_notify_key(pad->keypad,
					time_current_msecs(),
					code,
					WL_KEYBOARD_KEY_STATE_RELEASED);
		}

		if (tag == 9) {
			nemotimer_set_timeout(pad->timer, pad->timeout);
		}

		if (pad->borders[tag] != NULL) {
			nemoshow_item_set_fill_color(pad->borders[tag], 0x0, 0x0, 0x0, 0x0);
		}

		nemoactor_dispatch_frame(actor);

		nemograb_destroy(grab);

		return 0;
	}

	return 1;
}

static void nemopad_handle_actor_endgrab(struct wl_listener *listener, void *data)
{
	struct nemopad *pad = (struct nemopad *)container_of(listener, struct nemopad, actor_endgrab_listener);

	nemotimer_set_timeout(pad->timer, pad->timeout);
}

static void nemopad_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (nemotale_dispatch_grab(tale, event->device, type, event) == 0) {
			struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
			struct nemoshell *shell = NEMOSHOW_AT(show, shell);
			struct nemoactor *actor = NEMOSHOW_AT(show, actor);
			struct nemocompz *compz = shell->compz;
			struct nemopad *pad = (struct nemopad *)nemoshow_get_userdata(show);

			if (pad->is_pickable != 0) {
				if (nemotale_is_touch_down(tale, event, type)) {
					struct nemoseat *seat = compz->seat;

					nemotale_event_update_taps(tale, event, type);

					if (nemotale_is_single_tap(tale, event, type)) {
						struct touchpoint *tp;

						tp = nemoseat_get_touchpoint_by_id(seat, event->device);
						if (tp != NULL) {
							nemoshell_move_actor_by_touchpoint(shell, tp, actor);

							wl_list_remove(&pad->actor_endgrab_listener.link);
							wl_list_init(&pad->actor_endgrab_listener.link);

							pad->actor_endgrab_listener.notify = nemopad_handle_actor_endgrab;
							wl_signal_add(&actor->endgrab_signal, &pad->actor_endgrab_listener);

							nemotimer_set_timeout(pad->timer, 0);
						}
					} else if (nemotale_is_many_taps(tale, event, type)) {
						struct touchpoint *tp0, *tp1;

						nemotale_event_update_faraway_taps(tale, event);

						tp0 = nemoseat_get_touchpoint_by_id(seat, event->tap0->device);
						tp1 = nemoseat_get_touchpoint_by_id(seat, event->tap1->device);
						if (tp0 != NULL && tp1 != NULL) {
							nemoview_put_pivot(actor->view);

							nemoshell_pick_actor_by_touchpoint(shell, tp0, tp1, (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE), actor);

							wl_list_remove(&pad->actor_endgrab_listener.link);
							wl_list_init(&pad->actor_endgrab_listener.link);

							pad->actor_endgrab_listener.notify = nemopad_handle_actor_endgrab;
							wl_signal_add(&actor->endgrab_signal, &pad->actor_endgrab_listener);

							nemotimer_set_timeout(pad->timer, 0);
						}
					}
				}
			} else {
				uint32_t tag;
				int caps_on, shift_on;
				int i;

				tag = nemoshow_canvas_pick_tag(pad->canvas, event->x, event->y);

				if (nemotale_is_touch_down(tale, event, type)) {
					struct nemograb *grab;

					grab = nemograb_create(shell, tale, event, nemopad_dispatch_key_grab);
					nemograb_set_userdata(grab, pad);
					nemograb_set_tag(grab, tag);
					nemograb_check_signal(grab, &pad->destroy_signal);
					nemotale_dispatch_grab(tale, event->device, type, event);
				}

				caps_on = nemokeypad_is_caps_on(pad->keypad);
				shift_on = nemokeypad_is_shift_on(pad->keypad);

				if ((caps_on ^ shift_on) != pad->is_upper_case) {
					for (i = 0; i < NEMOPAD_KEYS_MAX; i++) {
						struct showone *path = pad->is_upper_case == 0 ? nemopaduppers[i] : nemopadnormals[i];
						uint32_t color = pad->is_upper_case == 0 ? nemopadkeys[i].color1 : nemopadkeys[i].color0;

						if (nemopaduppers[i] != NULL) {
							nemoshow_item_set_stroke_color(pad->borders[i],
									nemopadcolors[color][0],
									nemopadcolors[color][1],
									nemopadcolors[color][2],
									0xa0);
							nemoshow_item_set_fill_color(pad->keys[i],
									nemopadcolors[color][0],
									nemopadcolors[color][1],
									nemopadcolors[color][2],
									0xc0);

							nemoshow_item_path_clear(pad->keys[i]);
							nemoshow_item_path_append(pad->keys[i], path);
						}
					}

					nemoactor_dispatch_frame(actor);

					pad->is_upper_case = (caps_on ^ shift_on);
				}

				if (shift_on != pad->is_shift_case) {
					for (i = 0; i < NEMOPAD_KEYS_MAX; i++) {
						struct showone *path = pad->is_shift_case == 0 ? nemopadshifts[i] : nemopadnormals[i];
						uint32_t color = pad->is_shift_case == 0 ? nemopadkeys[i].color1 : nemopadkeys[i].color0;

						if (nemopadshifts[i] != NULL) {
							nemoshow_item_set_stroke_color(pad->borders[i],
									nemopadcolors[color][0],
									nemopadcolors[color][1],
									nemopadcolors[color][2],
									0xa0);
							nemoshow_item_set_fill_color(pad->keys[i],
									nemopadcolors[color][0],
									nemopadcolors[color][1],
									nemopadcolors[color][2],
									0xc0);

							nemoshow_item_path_clear(pad->keys[i]);
							nemoshow_item_path_append(pad->keys[i], path);
						}
					}

					nemoactor_dispatch_frame(actor);

					pad->is_shift_case = shift_on;
				}

				if (nemotale_is_single_click(tale, event, type)) {
					if (tag == 17) {
						nemopad_deactivate(pad);
					} else if (tag == 2 || tag == 16) {
						struct nemoview *view;
						float sx, sy;

						view = nemocompz_pick_canvas(compz, event->gx, event->gy, &sx, &sy);
						if (view != NULL && nemoview_support_touch_only(view) == 0) {
							nemokeypad_set_focus(pad->keypad, view);
						}
					}
				}
			}
		}
	}
}

static int nemopad_dispatch_actor_resize(struct nemoactor *actor, int32_t width, int32_t height, int32_t fixed)
{
	struct nemotale *tale = (struct nemotale *)actor->context;
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct talefbo *fbo = (struct talefbo *)nemotale_get_backend(tale);

	nemoactor_resize_gl(actor, width, height);

	nemoshow_set_size(show, width, height);

	nemoshow_render_one(show);

	nemotale_resize_fbo(fbo, width, height);

	nemotale_composite_fbo_full(tale);

	nemoactor_damage_dirty(actor);

	return 0;
}

static void nemopad_dispatch_actor_transform(struct nemoactor *actor, int32_t visible)
{
	struct nemotale *tale = (struct nemotale *)actor->context;
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct nemopad *pad = (struct nemopad *)nemoshow_get_userdata(show);

	pad->is_visible = visible;
}

static void nemopad_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct nemopad *pad = (struct nemopad *)data;

	if (pad->state & NEMOPAD_ACTIVE_STATE) {
		if (pad->is_visible != 0) {
			struct nemoshow *show = pad->show;
			struct nemoshell *shell = NEMOSHOW_AT(show, shell);
			struct nemoactor *actor = NEMOSHOW_AT(show, actor);
			int i;

			pad->is_pickable = 0;

			for (i = 0; i < NEMOPAD_KEYS_MAX; i++) {
				if (pad->keys[i] != NULL) {
					if (pad->is_upper_case != 0 || pad->is_shift_case != 0) {
						nemoshow_item_set_fill_color(pad->keys[i],
								nemopadcolors[nemopadkeys[i].color1][0],
								nemopadcolors[nemopadkeys[i].color1][1],
								nemopadcolors[nemopadkeys[i].color1][2],
								0xc0);
					} else {
						nemoshow_item_set_fill_color(pad->keys[i],
								nemopadcolors[nemopadkeys[i].color0][0],
								nemopadcolors[nemopadkeys[i].color0][1],
								nemopadcolors[nemopadkeys[i].color0][2],
								0xc0);
					}
				}
			}

			nemoactor_dispatch_frame(actor);
		}
	}
}

struct nemopad *nemopad_create(struct nemoshell *shell, uint32_t width, uint32_t height, uint32_t minwidth, uint32_t minheight, uint32_t timeout)
{
	struct nemocompz *compz = shell->compz;
	struct nemotimer *timer;
	struct nemopad *pad;

	pad = (struct nemopad *)malloc(sizeof(struct nemopad));
	if (pad == NULL)
		return NULL;
	memset(pad, 0, sizeof(struct nemopad));

	pad->shell = shell;

	nemosignal_init(&pad->destroy_signal);

	wl_list_init(&pad->actor_endgrab_listener.link);

	pad->timer = timer = nemotimer_create(compz);
	nemotimer_set_callback(timer, nemopad_dispatch_timer);
	nemotimer_set_userdata(timer, pad);

	pad->width = width;
	pad->height = height;
	pad->minwidth = minwidth;
	pad->minheight = minheight;
	pad->timeout = timeout;

	return pad;
}

void nemopad_destroy(struct nemopad *pad)
{
	nemosignal_emit(&pad->destroy_signal, pad);

	wl_list_remove(&pad->actor_endgrab_listener.link);

	nemotimer_destroy(pad->timer);

	if (pad->state & NEMOPAD_ACTIVE_STATE) {
		int i;

		nemoshow_one_destroy(pad->back);
		nemoshow_one_destroy(pad->canvas);
		nemoshow_one_destroy(pad->scene);

		for (i = 0; i < NEMOPAD_KEYS_MAX; i++) {
			if (pad->borders[i] != NULL)
				nemoshow_one_destroy(pad->borders[i]);
			if (pad->keys[i] != NULL)
				nemoshow_one_destroy(pad->keys[i]);
		}

		nemoshow_put_scene(pad->show);

		nemoshow_revoke_actor(pad->show);
		nemoshow_destroy_actor_on_idle(pad->show);

		nemokeypad_destroy(pad->keypad);
	}

	free(pad);
}

int nemopad_activate(struct nemopad *pad, double x, double y, double r)
{
	struct nemoshell *shell = pad->shell;
	struct nemocompz *compz = shell->compz;
	struct nemoactor *actor;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *frame;
	struct showone *set;
	int32_t basewidth = NEMOPAD_WIDTH;
	int32_t baseheight = NEMOPAD_HEIGHT;
	int i;

	pad->keypad = nemokeypad_create(compz->seat);
	if (pad->keypad == NULL)
		goto err1;

	pad->show = show = nemoshow_create_actor(shell,
			pad->width, pad->height,
			nemopad_dispatch_tale_event);
	if (show == NULL)
		goto err2;
	nemoshow_set_userdata(show, pad);

	pad->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, basewidth);
	nemoshow_scene_set_height(scene, baseheight);
	nemoshow_attach_one(show, scene);

	pad->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, basewidth);
	nemoshow_canvas_set_height(canvas, baseheight);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 0.0f);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	pad->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, basewidth);
	nemoshow_canvas_set_height(canvas, baseheight);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_event(canvas, 1);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	nemoshow_set_scene(show, scene);
	nemoshow_set_size(show, pad->width, pad->height);

	for (i = 0; i < NEMOPAD_KEYS_MAX; i++) {
		if (nemopadborders[i] != NULL) {
			pad->borders[i] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_attach_one(show, one);
			nemoshow_one_attach(canvas, one);
			nemoshow_one_set_tag(one, i);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one,
					nemopadkeys[i].x1 - nemopadkeys[i].x0);
			nemoshow_item_set_height(one,
					nemopadkeys[i].y1 - nemopadkeys[i].y0);
			nemoshow_item_set_filter(one, nemopadblur);
			nemoshow_item_set_stroke_color(one,
					nemopadcolors[nemopadkeys[i].color0][0],
					nemopadcolors[nemopadkeys[i].color0][1],
					nemopadcolors[nemopadkeys[i].color0][2],
					0xa0);
			nemoshow_item_set_stroke_width(one, nemopadkeys[i].thickness);
			nemoshow_item_set_alpha(one, 0.0f);
			nemoshow_item_set_tsr(one);
			nemoshow_item_translate(one, nemopadkeys[i].x0, nemopadkeys[i].y0);
			nemoshow_item_path_append(one, nemopadborders[i]);
		} else {
			pad->borders[i] = NULL;
		}

		if (nemopadnormals[i] != NULL) {
			pad->keys[i] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_attach_one(show, one);
			nemoshow_one_attach(canvas, one);
			nemoshow_one_set_tag(one, i);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one,
					nemopadkeys[i].x1 - nemopadkeys[i].x0);
			nemoshow_item_set_height(one,
					nemopadkeys[i].y1 - nemopadkeys[i].y0);
			nemoshow_item_set_filter(one, nemopadblur);
			nemoshow_item_set_fill_color(one,
					nemopadcolors[nemopadkeys[i].color0][0],
					nemopadcolors[nemopadkeys[i].color0][1],
					nemopadcolors[nemopadkeys[i].color0][2],
					0xc0);
			nemoshow_item_set_alpha(one, 0.0f);
			nemoshow_item_set_tsr(one);
			nemoshow_item_translate(one, nemopadkeys[i].x0, nemopadkeys[i].y0);
			nemoshow_item_path_append(one, nemopadnormals[i]);
		} else {
			pad->keys[i] = NULL;
		}

		frame = nemoshow_sequence_create_frame();
		nemoshow_sequence_set_timing(frame, 1.0f);

		if (pad->borders[i] != NULL) {
			set = nemoshow_sequence_create_set();
			nemoshow_sequence_set_source(set, pad->borders[i]);
			nemoshow_sequence_set_dattr(set, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
			nemoshow_one_attach(frame, set);
		}
		if (pad->keys[i] != NULL) {
			set = nemoshow_sequence_create_set();
			nemoshow_sequence_set_source(set, pad->keys[i]);
			nemoshow_sequence_set_dattr(set, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
			nemoshow_one_attach(frame, set);
		}

		sequence = nemoshow_sequence_create_easy(show, frame, NULL);

		trans = nemoshow_transition_create(nemopadease, random_get_int(500, 1000), random_get_int(0, 500));
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(show, trans);
	}

	nemoshow_render_one(show);

	pad->actor = actor = NEMOSHOW_AT(show, actor);
	nemoview_attach_layer(actor->view, &shell->overlay_layer);
	nemoview_set_state(actor->view, NEMO_VIEW_MAPPED_STATE);
	nemoview_set_position(actor->view, x - pad->width / 2.0f, y - 0.0f);
	nemoview_set_pivot(actor->view, pad->width / 2.0f, 0.0f);
	nemoview_set_flag(actor->view,
			(double)(nemopadkeys[9].x1 + nemopadkeys[9].x0) / 2.0f / (double)NEMOPAD_WIDTH,
			(double)(nemopadkeys[9].y1 + nemopadkeys[9].y0) / 2.0f / (double)NEMOPAD_HEIGHT);
	nemoview_set_rotation(actor->view, r);

	nemoactor_set_dispatch_resize(actor, nemopad_dispatch_actor_resize);
	nemoactor_set_dispatch_transform(actor, nemopad_dispatch_actor_transform);

	nemoactor_set_min_size(actor, pad->minwidth, pad->minheight);

	nemoactor_dispatch_frame(actor);

	pad->state |= NEMOPAD_ACTIVE_STATE;

	return 0;

err2:
	nemokeypad_destroy(pad->keypad);

err1:
	return -1;
}

static void nemopad_dispatch_deactivate_done(void *data)
{
	struct nemopad *pad = (struct nemopad *)data;

	nemopad_destroy(pad);
}

void nemopad_deactivate(struct nemopad *pad)
{
	struct nemoshow *show = pad->show;
	struct nemoactor *actor = NEMOSHOW_AT(show, actor);
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *frame;
	struct showone *set;
	uint32_t duration, delay;
	uint32_t ltime = 0;
	int i;

	if ((pad->state & NEMOPAD_DESTROY_STATE) != 0 ||
			(pad->state & NEMOPAD_ACTIVE_STATE) == 0)
		return;
	pad->state |= NEMOPAD_DESTROY_STATE;

	nemokeypad_set_focus(pad->keypad, NULL);

	frame = nemoshow_sequence_create_frame();
	nemoshow_sequence_set_timing(frame, 1.0f);

	for (i = 0; i < NEMOPAD_KEYS_MAX; i++) {
		if (pad->keys[i] != NULL) {
			set = nemoshow_sequence_create_set();
			nemoshow_sequence_set_source(set, pad->keys[i]);
			nemoshow_sequence_set_dattr(set, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
			nemoshow_one_attach(frame, set);
		}
	}

	sequence = nemoshow_sequence_create_easy(show, frame, NULL);

	trans = nemoshow_transition_create(nemopadease, 400, 0);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(show, trans);

	frame = nemoshow_sequence_create_frame();
	nemoshow_sequence_set_timing(frame, 1.0f);

	for (i = 0; i < NEMOPAD_KEYS_MAX; i++) {
		if (pad->borders[i] != NULL) {
			set = nemoshow_sequence_create_set();
			nemoshow_sequence_set_source(set, pad->borders[i]);
			nemoshow_sequence_set_dattr(set, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
			nemoshow_one_attach(frame, set);
		}
	}

	sequence = nemoshow_sequence_create_easy(show, frame, NULL);

	trans = nemoshow_transition_create(nemopadease, 800, 0);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(show, trans);

	nemoshow_transition_set_dispatch_done(trans, nemopad_dispatch_deactivate_done);
	nemoshow_transition_set_userdata(trans, pad);

	nemoactor_dispatch_frame(actor);

	nemoshow_revoke_actor(show);
}

void nemopad_set_focus(struct nemopad *pad, struct nemoview *view)
{
	nemokeypad_set_focus(pad->keypad, view);
}