#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <math.h>

#include <glripple.h>

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
	float *vectors;
	float *amplitudes;
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

	vectors = (float *)malloc(sizeof(float[3]) * rows * columns);
	nemofx_glripple_build_vectors(vectors, rows, columns, width, height);

	fprintf(stdout, "--- ripple vectors(rows: %d, columns: %d, width: %d, height: %d)\n", rows, columns, width, height);

	for (i = 0; i < columns; i++) {
		for (j = 0; j < rows; j++) {
			fprintf(stdout, "%g, %g, %f,\n",
					vectors[(i * rows + j) * 3 + 0],
					vectors[(i * rows + j) * 3 + 1],
					vectors[(i * rows + j) * 3 + 2]);
		}
	}

	fprintf(stdout, "\n");

	amplitudes = (float *)malloc(sizeof(float) * length);
	nemofx_glripple_build_amplitudes(amplitudes, length, cycles, amplitude);

	fprintf(stdout, "--- ripple amplitudes(length: %d, cycles: %d, amplitude: %f)\n", length, cycles, amplitude);

	for (i = 0; i < length; i++) {
		fprintf(stdout, "%g,\n", amplitudes[i]);
	}

	return 0;
}
