#ifndef	__NEMO_BOOK_H__
#define	__NEMO_BOOK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdio.h>
#include <stdint.h>

struct nemobook {
	void **lists;
	int nlists;

	uint32_t *types;
};

static inline struct nemobook *nemobook_create(int count)
{
	struct nemobook *book;

	book = (struct nemobook *)malloc(sizeof(struct nemobook));
	if (book == NULL)
		return NULL;
	memset(book, 0, sizeof(struct nemobook));

	book->lists = (void **)malloc(sizeof(void *) * count);
	memset(book->lists, 0, sizeof(void *) * count);

	book->types = (uint32_t *)malloc(sizeof(uint32_t) * count);
	memset(book->types, 0, sizeof(uint32_t) * count);

	book->nlists = count;

	return book;
}

static inline void nemobook_destroy(struct nemobook *book)
{
	free(book->types);
	free(book->lists);
	free(book);
}

static inline void nemobook_clear(struct nemobook *book)
{
	memset(book->lists, 0, sizeof(void *) * book->nlists);
}

static inline void nemobook_set_type(struct nemobook *book, int i, uint32_t type)
{
	book->types[i] = type;
}

static inline uint32_t nemobook_get_type(struct nemobook *book, int i)
{
	return book->types[i];
}

static inline void nemobook_set(struct nemobook *book, int i, void *v)
{
	book->lists[i] = v;
}

static inline void *nemobook_get(struct nemobook *book, int i)
{
	return book->lists[i];
}

static inline void nemobook_put(struct nemobook *book, int i)
{
	book->lists[i] = NULL;
}

static inline void nemobook_iset(struct nemobook *book, int i, int64_t v)
{
	book->lists[i] = (void *)v;
}

static inline int64_t nemobook_iget(struct nemobook *book, int i)
{
	return (int64_t)book->lists[i];
}

static inline int nemobook_is_empty(struct nemobook *book, int i)
{
	return book->lists[i] == NULL;
}

static inline int nemobook_needs_empty(struct nemobook *book)
{
	int i;

	for (i = 0; i < book->nlists; i++) {
		if (book->lists[i] == NULL)
			return i;
	}

	return -1;
}

static inline int nemobook_find_value(struct nemobook *book, void *v)
{
	int i;

	for (i = 0; i < book->nlists; i++) {
		if (book->lists[i] == v)
			return i;
	}

	return -1;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
