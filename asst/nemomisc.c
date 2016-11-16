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

uint64_t time_current_nsecs(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

int random_get_int(int min, int max)
{
	if (min == max)
		return min;

	return random() % (max - min + 1) + min;
}

double random_get_double(double min, double max)
{
	if (min == max)
		return min;

	return ((double)random() / (double)RAND_MAX) * (max - min) + min;
}
