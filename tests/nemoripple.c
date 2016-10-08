#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <math.h>

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "rows",				required_argument,		NULL,		'r' },
		{ "columns",		required_argument,		NULL,		'c' },
		{ "length",			required_argument,		NULL,		'l' },
		{ "width",			required_argument,		NULL,		'w' },
		{ "height",			required_argument,		NULL,		'h' },
		{ "cycle",			required_argument,		NULL,		'e' },
		{ "amplitude",	required_argument,		NULL,		'a' },
		{ 0 }
	};
	int rows = 32;
	int columns = 32;
	int length = 2048;
	int width = 400;
	int height = 400;
	int cycles = 18;
	float amplitude = 0.125f;
	float x, y, l;
	double t, a;
	int i, j;
	int opt;

	while (opt = getopt_long(argc, argv, "r:c:l:w:h:e:a:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'r':
				rows = strtoul(optarg, NULL, 10);
				break;

			case 'c':
				columns = strtoul(optarg, NULL, 10);
				break;

			case 'l':
				length = strtoul(optarg, NULL, 10);
				break;

			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'e':
				cycles = strtoul(optarg, NULL, 10);
				break;

			case 'a':
				amplitude = strtod(optarg, NULL);
				break;

			default:
				break;
		}
	}

	for (i = 0; i < columns; i++) {
		for (j = 0; j < rows; j++) {
			x = (float)i / (float)(columns - 1);
			y = (float)j / (float)(rows - 1);

			l = (float)sqrt(x * x + y * y);

			if (l == 0.0f) {
				x = 0.0f;
				y = 0.0f;
			} else {
				x /= l;
				y /= l;
			}

			fprintf(stdout, "%g, %g, %d,\n", x, y, (int)(l * width * 2));
		}
	}

	fprintf(stdout, "\n");

	for (i = 0; i < length; i++) {
		t = 1.0f - i / (length - 1.0f);
		a = (-cos(t * 2.0f * 3.1428571f * cycles) * 0.5f + 0.5f) * amplitude * t * t * t * t * t * t * t * t;
		if (i == 0)
			a = 0.0f;

		fprintf(stdout, "%g,\n", a);
	}

	return 0;
}
