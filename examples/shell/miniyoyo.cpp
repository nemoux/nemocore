#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <miniyoyo.h>
#include <showhelper.h>

struct miniyoyo *minishell_yoyo_create(void)
{
	struct miniyoyo *yoyo;

	yoyo = (struct miniyoyo *)malloc(sizeof(struct miniyoyo));
	if (yoyo == NULL)
		return NULL;
	memset(yoyo, 0, sizeof(struct miniyoyo));

	return yoyo;
}

void minishell_yoyo_destroy(struct miniyoyo *yoyo)
{
	free(yoyo);
}
