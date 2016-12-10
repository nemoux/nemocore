#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/sysinfo.h>

static unsigned int sysprocessors = 1;

void __attribute__((constructor(101))) sys_initialize(void)
{
	sysprocessors = get_nprocs();
}

void __attribute__((destructor(101))) sys_finalize(void)
{
}

int sys_get_process_name(pid_t pid, char *name, int size)
{
	FILE *fp;
	char procname[256];

	sprintf(procname, "/proc/%d/cmdline", pid);

	fp = fopen(procname, "r");
	if (fp != NULL) {
		size_t size;

		size = fread(name, sizeof(char), size, fp);
		if (size > 0)
			if (name[size - 1] == '\n')
				name[size - 1] = '\0';

		fclose(fp);

		return 1;
	}

	return 0;
}

int sys_get_process_parent_id(pid_t pid, pid_t *ppid)
{
	FILE *fp;
	char procname[256];

	sprintf(procname, "/proc/%d/stat", pid);

	fp = fopen(procname, "r");
	if (fp != NULL) {
		char buffer[1024];
		size_t size;
		int r = 0;

		size = fread(buffer, sizeof(char), sizeof(buffer), fp);
		if (size > 0) {
			char *ptr;
			char *tok;

			strtok_r(buffer, " ", &ptr);
			strtok_r(NULL, " ", &ptr);
			strtok_r(NULL, " ", &ptr);

			tok = strtok_r(NULL, " ", &ptr);
			if (tok != NULL) {
				*ppid = strtoul(tok, NULL, 10);

				r = 1;
			}
		}

		fclose(fp);

		return r;
	}

	return 0;
}

float sys_get_cpu_usage(void)
{
	struct sysinfo sinfo;

	sysinfo(&sinfo);

	return (float)sinfo.loads[0] / (float)(1 << SI_LOAD_SHIFT) / (float)sysprocessors;
}

float sys_get_memory_usage(void)
{
	struct sysinfo sinfo;

	sysinfo(&sinfo);

	return (float)(sinfo.totalram - sinfo.freeram) / (float)sinfo.totalram;
}
