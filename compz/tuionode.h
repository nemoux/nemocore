#ifndef __TUIO_NODE_H__
#define	__TUIO_NODE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <xmlparser.h>

#include <input.h>

typedef enum {
	NEMO_TUIO_NONE_STATE = 0,
	NEMO_TUIO_ALIVE_STATE = 1,
	NEMO_TUIO_SET_STATE = 2,
	NEMO_TUIO_FSEQ_STATE = 3,
	NEMO_TUIO_LAST_STATE
} NemoTuioState;

typedef enum {
	NEMO_TUIO_XPOS_VALUE = 0,
	NEMO_TUIO_YPOS_VALUE = 1,
	NEMO_TUIO_XVEL_VALUE = 2,
	NEMO_TUIO_YVEL_VALUE = 3,
	NEMO_TUIO_ACCEL_VALUE = 4,
	NEMO_TUIO_LAST_VALUE
} NemoTuioValue;

typedef enum {
	NEMO_TUIO_NONE_PROTOCOL = 0,
	NEMO_TUIO_XML_PROTOCOL = 1,
	NEMO_TUIO_OSC_PROTOCOL = 2,
	NEMO_TUIO_LAST_PROTOCOL
} NemoTuioProtocol;

struct nemocompz;
struct nemoscreen;

struct tuiotap {
	uint64_t id;

	float f[6];
};

struct tuionode {
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

extern struct tuionode *tuio_create_node(struct nemocompz *compz, int protocol, int port, int max);
extern void tuio_destroy_node(struct tuionode *node);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
