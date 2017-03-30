#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemonoty.h>
#include <nemomisc.h>

struct nemonoty *nemonoty_create(void)
{
	struct nemonoty *noty;

	noty = (struct nemonoty *)malloc(sizeof(struct nemonoty));
	if (noty == NULL)
		return NULL;
	memset(noty, 0, sizeof(struct nemonoty));

	nemolist_init(&noty->list);

	return noty;
}

void nemonoty_destroy(struct nemonoty *noty)
{
	struct notyone *one, *next;

	nemolist_for_each_safe(one, next, &noty->list, link) {
		nemolist_remove(&one->link);

		free(one);
	}

	free(noty);
}

void nemonoty_attach(struct nemonoty *noty, nemonoty_dispatch_t dispatch, void *data)
{
	struct notyone *one;

	one = (struct notyone *)malloc(sizeof(struct notyone));
	one->dispatch = dispatch;
	one->data = data;

	nemolist_insert(&noty->list, &one->link);
}

void nemonoty_detach(struct nemonoty *noty, nemonoty_dispatch_t dispatch, void *data)
{
	struct notyone *one;

	nemolist_for_each(one, &noty->list, link) {
		if (one->dispatch == dispatch && one->data == data) {
			nemolist_remove(&one->link);

			free(one);

			break;
		}
	}
}

void nemonoty_dispatch(struct nemonoty *noty, void *event)
{
	struct notyone *one;

	nemolist_for_each(one, &noty->list, link) {
		if (one->dispatch(one->data, event) != 0)
			break;
	}
}
