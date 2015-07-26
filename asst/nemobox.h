#ifndef	__NEMO_BOX_H__
#define	__NEMO_BOX_H__

#include <stdio.h>

#define	NEMOBOX_APPEND(a, s, n, v)	\
	if (n >= s) {	\
		a = (__typeof__(a))realloc(a, sizeof(a[0]) * s * 2);	\
		s = s * 2;	\
	}	\
	a[n++] = v

#define	NEMOBOX_APPEND_BUFFER(a, s, n, b, l)	\
	if (n + l >= s) {	\
		a = (__typeof__(a))realloc(a, sizeof(a[0]) * (s + l));	\
		s = s + l;	\
	}	\
	memcpy(&a[n], b, sizeof(a[0]) * l);	\
	n = n + l

#define	NEMOBOX_APPEND_STRING(a, s, n, b, l)	\
	if (n + l + 1 >= s) {	\
		a = (__typeof__(a))realloc(a, sizeof(a[0]) * (s + l + 1));	\
		s = s + l + 1;	\
	}	\
	memcpy(&a[n], b, sizeof(a[0]) * l);	\
	a[n + l] = '\0';	\
	n = n + l;

#define	NEMOBOX_REMOVE(a, n, i)	\
	if (i < n - 1) {	\
		memcpy(a + i, a + i + 1, sizeof(a[0]) * (n - i - 1));	\
	}	\
	n--

#define	NEMOBOX_BSEARCH(a, n, t, c)	\
	bsearch(t, a, n, sizeof(a[0]), c)

#define	NEMOBOX_QSORT(a, n, c)	\
	qsort(a, n, sizeof(a[0]), c)

#endif
