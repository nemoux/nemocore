#ifndef	__NEMO_LIST_H__
#define	__NEMO_LIST_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemolist {
	struct nemolist *prev;
	struct nemolist *next;

	void *data;
};

#ifndef offsetof
#	define offsetof(type, member) \
	((char *)&((type *)0)->member - (char *)0)
#endif

#ifndef container_of
#	define container_of(ptr, type, member) ({				\
		const __typeof__(((type *)0)->member) *__mptr = (ptr);	\
		(type *)((char *)__mptr - offsetof(type, member));})
#endif

#define nemo_container_of(ptr, sample, member)				\
	(__typeof__(sample))((char *)(ptr) - offsetof(__typeof__(*sample), member))

#define nemo_type_of(ptr, type, member)				\
	(type *)((char *)(ptr) - offsetof(type, member))

#define nemolist_for_each(pos, head, member)				\
	for (pos = nemo_container_of((head)->next, pos, member);	\
			&pos->member != (head);					\
			pos = nemo_container_of(pos->member.next, pos, member))

#define nemolist_for_each_safe(pos, tmp, head, member)			\
	for (pos = nemo_container_of((head)->next, pos, member),		\
			tmp = nemo_container_of((pos)->member.next, tmp, member);	\
			&pos->member != (head);					\
			pos = tmp,							\
			tmp = nemo_container_of((pos)->member.next, tmp, member))

#define nemolist_for_each_reverse(pos, head, member)			\
	for (pos = nemo_container_of((head)->prev, pos, member);	\
			&pos->member != (head);					\
			pos = nemo_container_of((pos)->member.prev, pos, member))

#define nemolist_for_each_reverse_safe(pos, tmp, head, member)		\
	for (pos = nemo_container_of((head)->prev, pos, member),	\
			tmp = nemo_container_of((pos)->member.prev, tmp, member);	\
			&pos->member != (head);					\
			pos = tmp,							\
			tmp = nemo_container_of((pos)->member.prev, tmp, member))

#define nemolist_node_for_each(pos, head)	\
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define nemolist_node_for_each_safe(pos, tmp, head)	\
	for (pos = (head)->next, tmp = pos->next; pos != (head); pos = tmp, tmp = pos->next)

#define nemolist_node_for_each_reverse(pos, head)	\
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define nemolist_node_for_each_reverse_safe(pos, tmp, head)	\
	for (pos = (head)->prev, tmp = pos->prev; pos != (head); pos = tmp, tmp = pos->prev)

#define	nemolist_node0(head, type, member)	\
	(head)->next == NULL ? NULL : nemo_type_of((head)->next, type, member)

#define	nemolist_node1(head, type, member)	\
	(head)->next == NULL ? NULL : (head)->next->next == NULL ? NULL : nemo_type_of((head)->next->next, type, member)

#define	nemolist_node2(head, type, member)	\
	(head)->next == NULL ? NULL : (head)->next->next == NULL ? NULL : (head)->next->next->next == NULL ? NULL : nemo_type_of((head)->next->next->next, type, member)

#define	nemolist_next(head, pos, type, member)	\
	(pos)->member.next == (head) ? NULL : nemo_type_of((pos)->member.next, type, member)

#define	nemolist_prev(head, pos, type, member)	\
	(pos)->member.prev == (head) ? NULL : nemo_type_of((pos)->member.prev, type, member)

static inline void nemolist_init(struct nemolist *list)
{
	list->prev = list;
	list->next = list;
}

static inline void nemolist_insert(struct nemolist *list, struct nemolist *elm)
{
	elm->prev = list;
	elm->next = list->next;
	list->next = elm;
	elm->next->prev = elm;
}

static inline void nemolist_insert_tail(struct nemolist *list, struct nemolist *elm)
{
	elm->prev = list->prev;
	elm->next = list;
	list->prev = elm;
	elm->prev->next = elm;
}

static inline void nemolist_remove(struct nemolist *elm)
{
	elm->prev->next = elm->next;
	elm->next->prev = elm->prev;
	elm->next = NULL;
	elm->prev = NULL;
}

static inline int nemolist_length(const struct nemolist *list)
{
	struct nemolist *e = list->next;
	int count;

	for (count = 0; e != list; count++) {
		e = e->next;
	}

	return count;
}

static inline int nemolist_empty(const struct nemolist *list)
{
	return list->next == list;
}

static inline void nemolist_insert_list(struct nemolist *list, struct nemolist *other)
{
	if (nemolist_empty(other))
		return;

	other->next->prev = list;
	other->prev->next = list->next;
	list->next->prev = other->prev;
	list->next = other->next;
}

static inline void nemolist_enqueue(struct nemolist *list, struct nemolist *elm)
{
	nemolist_insert(list, elm);
}

static inline void nemolist_enqueue_tail(struct nemolist *list, struct nemolist *elm)
{
	nemolist_insert_tail(list, elm);
}

static inline struct nemolist *nemolist_dequeue(struct nemolist *list)
{
	struct nemolist *elm;

	if (nemolist_empty(list))
		return NULL;

	elm = list->prev;

	nemolist_remove(elm);
	nemolist_init(elm);

	return elm;
}

static inline void nemolist_push(struct nemolist *list, struct nemolist *elm)
{
	nemolist_insert_tail(list, elm);
}

static inline struct nemolist *nemolist_pop(struct nemolist *list)
{
	struct nemolist *elm;

	if (nemolist_empty(list))
		return NULL;

	elm = list->prev;

	nemolist_remove(elm);
	nemolist_init(elm);

	return elm;
}

static inline struct nemolist *nemolist_peek_head(struct nemolist *list)
{
	if (nemolist_empty(list))
		return NULL;

	return list->next;
}

static inline struct nemolist *nemolist_peek_tail(struct nemolist *list)
{
	if (nemolist_empty(list))
		return NULL;

	return list->prev;
}

static inline void nemolist_set_data(struct nemolist *list, void *data)
{
	list->data = data;
}

static inline void *nemolist_get_data(struct nemolist *list)
{
	return list->data;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
