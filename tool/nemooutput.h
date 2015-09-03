#ifndef	__NEMOTOOL_OUTPUT_H__
#define	__NEMOTOOL_OUTPUT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemotool.h>
#include <nemolist.h>
#include <nemolistener.h>

struct nemooutput {
	struct nemotool *tool;

	struct wl_output *output;
	uint32_t id;

	int transform;
	int scale;
	char *make;
	char *model;

	int32_t x, y;
	int32_t width, height;
	int32_t mmwidth, mmheight;

	int32_t refresh;

	struct nemolist link;

	void *userdata;
};

extern struct nemooutput *nemooutput_create(struct nemotool *tool, uint32_t id);
extern void nemooutput_destroy(struct nemooutput *output);

extern int nemooutput_register(struct nemotool *tool, uint32_t id);
extern void nemooutput_unregister(struct nemotool *tool, uint32_t id);

extern struct nemooutput *nemooutput_find(struct nemotool *tool, struct wl_output *_output);

static inline void nemooutput_set_userdata(struct nemooutput *output, void *data)
{
	output->userdata = data;
}

static inline void *nemooutput_get_userdata(struct nemooutput *output)
{
	return output->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
