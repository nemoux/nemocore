#ifndef __NEMO_TUIO_H__
#define __NEMO_TUIO_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <xmlhelper.h>

#include <input.h>

typedef enum {
	NEMOTUIO_NONE_STATE = 0,
	NEMOTUIO_ALIVE_STATE = 1,
	NEMOTUIO_SET_STATE = 2,
	NEMOTUIO_FSEQ_STATE = 3,
	NEMOTUIO_LAST_STATE
} NemoTuioState;

typedef enum {
	NEMOTUIO_XPOS_VALUE = 0,
	NEMOTUIO_YPOS_VALUE = 1,
	NEMOTUIO_XVEL_VALUE = 2,
	NEMOTUIO_YVEL_VALUE = 3,
	NEMOTUIO_ACCEL_VALUE = 4,
	NEMOTUIO_LAST_VALUE
} NemoTuioValue;

typedef enum {
	NEMOTUIO_NONE_PROTOCOL = 0,
	NEMOTUIO_XML_PROTOCOL = 1,
	NEMOTUIO_OSC_PROTOCOL = 2,
	NEMOTUIO_LAST_PROTOCOL
} NemoTuioProtocol;

struct nemocompz;
struct nemoscreen;

struct tuiotap {
	uint64_t id;

	float f[6];
};

struct tuio {
	struct inputnode base;

	struct nemocompz *compz;

	struct nemotouch *touch;

	int protocol;

	int state;

	struct tuiotap *taps;

	struct wl_list link;

	struct {
		int index;
	} alive;

	struct {
		struct tuiotap *tap;
		int id;
		int index;
	} set;

	struct {
		int id;
	} fseq;

	int fd;
	struct wl_event_source *source;

	struct xmlparser *xmlparser;
};

extern struct tuio *tuio_create(struct nemocompz *compz, int protocol, int port, int max);
extern void tuio_destroy(struct tuio *tuio);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
