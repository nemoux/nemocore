#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <namespacehelper.h>

int namespace_has_prefix(const char *ns, const char *ps)
{
	int length = strlen(ps);
	int i;

	for (i = 0; i < length; i++) {
		if (ns[i] == '\0' || ns[i] != ps[i])
			return 0;
	}

	return 1;
}
