#ifndef	__NEMOTALE_H__
#define	__NEMOTALE_H__

#include <stdint.h>
#include <pixman.h>
#include <cairo.h>

#include <nemolist.h>
#include <nemolistener.h>
#include <nemomatrix.h>
#include <nemobox.h>

struct nemotale;
struct talenode;
struct taleevent;

typedef void (*nemotale_dispatch_event_t)(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event);

struct nemotale {
	void *pmcontext;
	void *glcontext;
	void *backend;

	void *userdata;

	struct talenode **nodes;
	int nnodes, snodes;

	struct nemosignal destroy_signal;

	int32_t width, height, stride;
	pixman_region32_t region;
	pixman_region32_t input;
	pixman_region32_t damage;

	pixman_format_code_t read_format;

	struct nemomatrix matrix;

	struct nemolist ptap_list;
	struct nemolist tap_list;
	struct nemolist grab_list;

	nemotale_dispatch_event_t dispatch_event;
};

#define	NEMOTALE_DESTROY_SIGNAL(tale)		(&tale->destroy_signal)
#define	NEMOTALE_DAMAGE(tale)						(&tale->damage)

extern int nemotale_prepare(struct nemotale *tale);
extern void nemotale_finish(struct nemotale *tale);

extern struct talenode *nemotale_pick(struct nemotale *tale, float x, float y, float *sx, float *sy);

extern void nemotale_damage_region(struct nemotale *tale, pixman_region32_t *region);
extern void nemotale_damage_below(struct nemotale *tale, struct talenode *node);
extern void nemotale_damage_all(struct nemotale *tale);
extern void nemotale_attach_node(struct nemotale *tale, struct talenode *node);
extern void nemotale_detach_node(struct nemotale *tale, struct talenode *node);

extern void nemotale_prepare_composite(struct nemotale *tale);
extern void nemotale_finish_composite(struct nemotale *tale, pixman_region32_t *region);

static inline int nemotale_get_node_count(struct nemotale *tale)
{
	return tale->nnodes;
}

static inline int32_t nemotale_get_width(struct nemotale *tale)
{
	return tale->width;
}

static inline int32_t nemotale_get_height(struct nemotale *tale)
{
	return tale->height;
}

static inline void nemotale_set_dispatch_event(struct nemotale *tale, nemotale_dispatch_event_t dispatch)
{
	tale->dispatch_event = dispatch;
}

static inline void nemotale_set_backend(struct nemotale *tale, void *backend)
{
	tale->backend = backend;
}

static inline void *nemotale_get_backend(struct nemotale *tale)
{
	return tale->backend;
}

static inline void nemotale_set_userdata(struct nemotale *tale, void *data)
{
	tale->userdata = data;
}

static inline void *nemotale_get_userdata(struct nemotale *tale)
{
	return tale->userdata;
}

#endif
