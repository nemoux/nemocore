#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <nemoscope.h>
#include <nemomisc.h>

int main(void)
{
	struct nemoscope *scope;

	scope = nemoscope_create();

	nemoscope_add_triangle(scope, 1, 50, 25, 25, 75, 75, 75);
	nemoscope_add_cmd(scope, 2, "p:50,0:0,50:50,100:100,50");
	nemoscope_add_circle(scope, 3, 50, 50, 50);

	NEMO_DEBUG("(50, 50) %d\n", nemoscope_pick(scope, 50, 50));
	NEMO_DEBUG("(100, 50) %d\n", nemoscope_pick(scope, 100, 50));
	NEMO_DEBUG("(100, 55) %d\n", nemoscope_pick(scope, 100, 55));

	nemoscope_destroy(scope);

	return 0;
}
