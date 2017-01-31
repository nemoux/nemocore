#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomotz.h>
#include <nemomisc.h>

struct nemomotz *nemomotz_create(void)
{
	struct nemomotz *motz;

	motz = (struct nemomotz *)malloc(sizeof(struct nemomotz));
	if (motz == NULL)
		return NULL;
	memset(motz, 0, sizeof(struct nemomotz));

	return motz;
}

void nemomotz_destroy(struct nemomotz *motz)
{
	free(motz);
}
