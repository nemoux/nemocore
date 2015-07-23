#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <iconv.h>

#include <utf8helper.h>

#ifdef WORDS_BIGENDIAN
#	define UCS4 "UCS-4BE"
#else
#	define UCS4 "UCS-4LE"
#endif

#ifndef ICONV_CONST
#	define ICONV_CONST
#endif

int utf8_from_ucs4(char *dst, const uint32_t *src, size_t size)
{
	ICONV_CONST char *inbuf;
	char *outbuf;
	size_t length;
	size_t inleft, outleft;
	iconv_t cv;

	for (length = 0; src[length] != 0; length++)
		continue;

	if (length == 0) {
		dst[0] = '\0';
		return 0;
	}

	cv = iconv_open("UTF-8", UCS4);
	if (cv == (iconv_t)(-1))
		return -1;

	inbuf = (char *)src;
	inleft = length * 4;
	outbuf = dst;
	outleft = size;

	iconv(cv, &inbuf, &inleft, &outbuf, &outleft);

	iconv_close(cv);

	if (outleft > 0)
		*outbuf = '\0';
	else
		dst[size - 1] = '\0';

	return size - outleft;
}

const char *utf8_prev_char(const char *s, const char *p)
{
	for (--p; p >= s; --p) {
		if ((*p & 0xc0) != 0x80)
			return p;
	}

	return NULL;
}

const char *utf8_next_char(const char *p)
{
	if (*p++ != 0) {
		while ((*p & 0xc0) == 0x80)
			p++;
		return p;
	}

	return NULL;
}

int utf8_length(const char *p)
{
	int length = 0;

	for (; *p != '\0'; p++) {
		if ((*p & 0xc0) != 0x80)
			length++;
	}

	return length;
}
