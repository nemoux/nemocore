#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <tesseract/capi.h>
#include <leptonica/allheaders.h>

#include <nemomisc.h>

struct nemopen {
	int width, height;

	TessBaseAPI *tess;
};

int nemopen_initialize(struct nemopen *pen, const char *language)
{
	TessBaseAPI *tess;

	tess = TessBaseAPICreate();

	if (TessBaseAPIInit3(tess, NULL, language) != 0)
		goto err1;

	pen->tess = tess;

	return 0;

err1:
	TessBaseAPIEnd(tess);
	TessBaseAPIDelete(tess);

	return -1;
}

void nemopen_finalize(struct nemopen *pen)
{
	TessBaseAPIEnd(pen->tess);
	TessBaseAPIDelete(pen->tess);
}

int nemopen_recognize(struct nemopen *pen, const char *filepath)
{
	PIX *img;
	char *text;

	img = pixRead(filepath);
	if (img == NULL)
		return -1;

	TessBaseAPISetImage2(pen->tess, img);

	if (TessBaseAPIRecognize(pen->tess, NULL) == 0 && (text = TessBaseAPIGetUTF8Text(pen->tess)) != NULL) {
		NEMO_DEBUG("=> [%s]\n", text);

		TessDeleteText(text);
	}

	pixDestroy(&img);

	return 0;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",				required_argument,		NULL,		'w' },
		{ "height",				required_argument,		NULL,		'h' },
		{ "content",			required_argument,		NULL,		'c' },
		{ 0 }
	};

	struct nemopen *pen;
	char *contentpath = NULL;
	int width = 500;
	int height = 500;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:c:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'c':
				contentpath = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (contentpath == NULL)
		return 0;

	pen = (struct nemopen *)malloc(sizeof(struct nemopen));
	if (pen == NULL)
		return -1;
	memset(pen, 0, sizeof(struct nemopen));

	pen->width = width;
	pen->height = height;

	nemopen_initialize(pen, "eng");

	nemopen_recognize(pen, contentpath);

	nemopen_finalize(pen);

	free(pen);

	return 0;
}
