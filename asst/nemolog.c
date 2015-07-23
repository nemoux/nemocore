#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <sys/time.h>

#include <nemolog.h>

static FILE *nemologfile = NULL;

int nemolog_open_file(const char *filepath)
{
	FILE *file;

	file = fopen(filepath, "a");
	if (file == NULL)
		return -1;

	nemologfile = file;

	return 0;
}

void nemolog_close_file(void)
{
	fclose(nemologfile);

	nemologfile = NULL;
}

void nemolog_set_file(FILE *file)
{
	nemologfile = file;
}

static int nemolog_write(const char *syntax, const char *tag, const char *fmt, va_list vargs)
{
	struct timeval tv;
	struct tm *tm;
	char buffer[128] = "";
	int r = 0;

	gettimeofday(&tv, NULL);

	tm = localtime(&tv.tv_sec);
	if (tm != NULL) {
		strftime(buffer, sizeof(buffer), "%H:%M:%S", tm);
	}

	r += fprintf(nemologfile, syntax, tag, buffer, tv.tv_usec / 1000);
	r += vfprintf(nemologfile, fmt, vargs);

	return r;
}

int nemolog_message(const char *tag, const char *fmt, ...)
{
	va_list vargs;
	int r;

	if (nemologfile == NULL)
		return 0;

	va_start(vargs, fmt);
	r = nemolog_write("\e[32;1mNEMO:\e[m \e[1;33m[%s] (%s+%03li)\e[0m ", tag, fmt, vargs);
	va_end(vargs);

	return r;
}

int nemolog_warning(const char *tag, const char *fmt, ...)
{
	va_list vargs;
	int r;

	if (nemologfile == NULL)
		return 0;

	va_start(vargs, fmt);
	r = nemolog_write("\e[32;1mNEMO:\e[m \e[1;30m[%s] (%s+%03li)\e[0m ", tag, fmt, vargs);
	va_end(vargs);

	return r;
}

int nemolog_error(const char *tag, const char *fmt, ...)
{
	va_list vargs;
	int r;

	if (nemologfile == NULL)
		return 0;

	va_start(vargs, fmt);
	r = nemolog_write("\e[32;1mNEMO:\e[m \e[1;31m[%s] (%s+%03li)\e[0m ", tag, fmt, vargs);
	va_end(vargs);

	return r;
}
