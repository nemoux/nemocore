#ifndef	__NEMOSOUND_H__
#define	__NEMOSOUND_H__

#include <nemotool.h>

struct nemosound {
	struct nemotool *tool;

	struct nemo_sound_manager *manager;
};

#endif
