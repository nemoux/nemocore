#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <hangul-1.0/hangul.h>

#include <nemohangul.h>
#include <nemomisc.h>

struct nemohangul {
	HangulInputContext *context;
};

struct nemohangul *nemohangul_create(void)
{
	struct nemohangul *hangul;

	hangul = (struct nemohangul *)malloc(sizeof(struct nemohangul));
	if (hangul == NULL)
		return NULL;
	memset(hangul, 0, sizeof(struct nemohangul));

	hangul->context = hangul_ic_new("2");
	if (hangul->context == NULL)
		goto err1;

	return hangul;

err1:
	free(hangul);

	return NULL;
}

void nemohangul_destroy(struct nemohangul *hangul)
{
	hangul_ic_delete(hangul->context);

	free(hangul);
}

void nemohangul_process(struct nemohangul *hangul, int code)
{
	hangul_ic_process(hangul->context, code);
}

void nemohangul_backspace(struct nemohangul *hangul)
{
	if (hangul_ic_is_empty(hangul->context) == 0)
		hangul_ic_backspace(hangul->context);
}

void nemohangul_reset(struct nemohangul *hangul)
{
	hangul_ic_reset(hangul->context);
}

void nemohangul_delete(struct nemohangul *hangul)
{
	hangul_ic_delete(hangul->context);
}

const uint32_t *nemohangul_get_preedit_string(struct nemohangul *hangul)
{
	return hangul_ic_get_preedit_string(hangul->context);
}

const uint32_t *nemohangul_get_commit_string(struct nemohangul *hangul)
{
	return hangul_ic_get_commit_string(hangul->context);
}

const uint32_t *nemohangul_flush(struct nemohangul *hangul)
{
	return hangul_ic_flush(hangul->context);
}

int nemohangul_is_empty(struct nemohangul *hangul)
{
	return hangul_ic_is_empty(hangul->context);
}
