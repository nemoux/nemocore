#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>
#include <time.h>
#include <sys/time.h>

#ifdef NEMOUX_WITH_UNWIND
#define	UNW_LOCAL_ONLY
#include <libunwind.h>
#endif

#include <nemomisc.h>

void debug_show_backtrace(void)
{
#ifdef NEMOUX_WITH_UNWIND
	unw_context_t context;
	unw_cursor_t cursor;
	unw_word_t off, ip, sp;
	unw_proc_info_t pip;
	char procname[256];
	int ret;

	if (unw_getcontext(&context))
		return;

	if (unw_init_local(&cursor, &context))
		return;

	while (unw_step(&cursor) > 0) {
		if (unw_get_proc_info(&cursor, &pip))
			break;

		ret = unw_get_proc_name(&cursor, procname, sizeof(procname), &off);
		if (ret && ret != -UNW_ENOMEM) {
			procname[0] = '?';
			procname[1] = 0;
		}

		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);

		NEMO_DEBUG("ip = 0x%lx (%s), sp = 0x%lx\n", (long)ip, procname, (long)sp);
	}
#endif
}

uint32_t time_current_msecs(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int random_get_int(int min, int max)
{
	if (min == max)
		return min;

	return random() % (max - min) + min;
}

double random_get_double(double min, double max)
{
	if (min == max)
		return min;

	return ((double)random() / (double)RAND_MAX) * (max - min) + min;
}

double point_get_distance(double x0, double y0, double x1, double y1)
{
	double dx = x1 - x0;
	double dy = y1 - y0;
	
	return sqrtf(dx * dx + dy * dy);
}
