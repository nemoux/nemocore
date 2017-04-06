#ifndef	__NEMOMOTZ_H__
#define	__NEMOMOTZ_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>
#include <nemolistener.h>
#include <nemotransition.h>
#include <nemotozz.h>

typedef enum {
	NEMOMOTZ_REDRAW_FLAG = (1 << 0)
} NemoMotzFlag;

typedef enum {
	NEMOMOTZ_ONE_FLAGS_DIRTY = (1 << 0)
} NemoMotzOneDirty;

struct nemomotz;
struct motzone;
struct motztap;

typedef void (*nemomotz_down_t)(struct nemomotz *motz, struct motztap *tap, float x, float y);
typedef void (*nemomotz_motion_t)(struct nemomotz *motz, struct motztap *tap, float x, float y);
typedef void (*nemomotz_up_t)(struct nemomotz *motz, struct motztap *tap, float x, float y);

typedef void (*nemomotz_one_draw_t)(struct nemomotz *motz, struct motzone *one);
typedef void (*nemomotz_one_down_t)(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y);
typedef void (*nemomotz_one_motion_t)(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y);
typedef void (*nemomotz_one_up_t)(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y);

typedef struct motzone *(*nemomotz_one_contain_t)(struct motzone *one, float x, float y);
typedef void (*nemomotz_one_update_t)(struct motzone *one);
typedef int (*nemomotz_one_frame_t)(struct motzone *one, uint32_t msecs);
typedef void (*nemomotz_one_destroy_t)(struct motzone *one);

struct nemomotz {
	struct nemolist one_list;
	struct nemolist tap_list;

	struct transitiongroup *transitions;

	struct nemotozz *tozz;

	int width, height;

	struct {
		int width, height;
	} viewport;

	uint32_t flags;

	nemomotz_down_t down;
	nemomotz_motion_t motion;
	nemomotz_up_t up;

	void *data;
};

struct motzone {
	nemomotz_one_draw_t draw;
	nemomotz_one_down_t down;
	nemomotz_one_motion_t motion;
	nemomotz_one_up_t up;
	nemomotz_one_contain_t contain;
	nemomotz_one_update_t update;
	nemomotz_one_frame_t frame;
	nemomotz_one_destroy_t destroy;

	uint32_t flags;
	uint32_t dirty;

	struct nemolist link;
	struct nemosignal destroy_signal;

	struct nemolist one_list;
};

struct motztap {
	uint64_t id;

	struct motzone *one;
	struct nemolistener destroy_listener;

	struct nemolist link;
};

#define NEMOMOTZ_DECLARE_SET_ATTRIBUTE(tag, type, attr, name, dirty)	\
	static inline void nemomotz_##tag##_set_##name(struct motzone *one, type name) {	\
		struct motz##tag *tag = ((struct motz##tag *)container_of(one, struct motz##tag, one));	\
		tag->attr = name;	\
		nemomotz_one_set_dirty(one, dirty);	\
	}
#define NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(tag, type, attr, name, dirty, flags)	\
	static inline void nemomotz_##tag##_set_##name(struct motzone *one, type name) {	\
		struct motz##tag *tag = ((struct motz##tag *)container_of(one, struct motz##tag, one));	\
		tag->attr = name;	\
		nemomotz_one_set_dirty(one, dirty);	\
		nemomotz_one_set_flags(one, flags);	\
	}
#define NEMOMOTZ_DECLARE_GET_ATTRIBUTE(tag, type, attr, name)	\
	static inline type nemomotz_##tag##_get_##name(struct motzone *one) {	\
		struct motz##tag *tag = ((struct motz##tag *)container_of(one, struct motz##tag, one));	\
		return tag->attr;	\
	}
#define NEMOMOTZ_DECLARE_DUP_STRING(tag, attr, name, dirty)	\
	static inline void nemomotz_##tag##_set_##name(struct motzone *one, const char *attr) {	\
		struct motz##tag *tag = ((struct motz##tag *)container_of(one, struct motz##tag, one));	\
		if (tag->attr != NULL) free(tag->attr);	\
		tag->attr = strdup(attr);	\
		nemomotz_one_set_dirty(one, dirty);	\
	}
#define NEMOMOTZ_DECLARE_COPY_STRING(tag, attr, name, dirty)	\
	static inline void nemomotz_##tag##_set_##name(struct motzone *one, const char *attr) {	\
		struct motz##tag *tag = ((struct motz##tag *)container_of(one, struct motz##tag, one));	\
		strcpy(tag->attr, attr);	\
		nemomotz_one_set_dirty(one, dirty);	\
	}
#define NEMOMOTZ_DECLARE_SET_TRANSITION(tag, attr, name, _dirty)	\
	static inline void nemomotz_transition_##tag##_set_##name(struct nemotransition *trans, int index, struct motzone *one) {	\
		struct motz##tag *tag = ((struct motz##tag *)container_of(one, struct motz##tag, one));	\
		nemotransition_set_float_with_dirty(trans, index, &tag->attr, tag->attr, &one->dirty, _dirty);	\
	}
#define NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(tag, attr, name, _dirty, _flags)	\
	static inline void nemomotz_transition_##tag##_set_##name(struct nemotransition *trans, int index, struct motzone *one) {	\
		struct motz##tag *tag = ((struct motz##tag *)container_of(one, struct motz##tag, one));	\
		nemomotz_one_set_flags(one, _flags);	\
		nemotransition_set_float_with_dirty(trans, index, &tag->attr, tag->attr, &one->dirty, _dirty);	\
	}
#define NEMOMOTZ_DECLARE_CHECK_TRANSITION_DESTROY(tag)	\
	static inline void nemomotz_transition_##tag##_check_destroy(struct nemotransition *trans, struct motzone *one) {	\
		nemotransition_check_object_destroy(trans, &one->destroy_signal);	\
	}
#define NEMOMOTZ_DECLARE_CHECK_TRANSITION_REVOKE(tag)	\
	static inline void nemomotz_transition_##tag##_check_revoke(struct nemotransition *trans, struct motzone *one) {	\
		nemotransition_check_object_revoke(trans, &one->destroy_signal, one, sizeof(struct motz##tag));	\
	}

extern struct nemomotz *nemomotz_create(void);
extern void nemomotz_destroy(struct nemomotz *motz);

extern void nemomotz_set_size(struct nemomotz *motz, int width, int height);

extern void nemomotz_update(struct nemomotz *motz, uint32_t msecs);

extern int nemomotz_attach_buffer(struct nemomotz *motz, void *buffer, int width, int height);
extern void nemomotz_detach_buffer(struct nemomotz *motz);
extern void nemomotz_update_buffer(struct nemomotz *motz);
extern void nemomotz_clear_buffer(struct nemomotz *motz);

extern void nemomotz_attach_one(struct nemomotz *motz, struct motzone *one);
extern void nemomotz_detach_one(struct nemomotz *motz, struct motzone *one);

extern struct motzone *nemomotz_pick_one(struct nemomotz *motz, float x, float y);

extern void nemomotz_dispatch_down_event(struct nemomotz *motz, uint64_t id, float x, float y);
extern void nemomotz_dispatch_motion_event(struct nemomotz *motz, uint64_t id, float x, float y);
extern void nemomotz_dispatch_up_event(struct nemomotz *motz, uint64_t id, float x, float y);

extern void nemomotz_down_null(struct nemomotz *motz, struct motztap *tap, float x, float y);
extern void nemomotz_motion_null(struct nemomotz *motz, struct motztap *tap, float x, float y);
extern void nemomotz_up_null(struct nemomotz *motz, struct motztap *tap, float x, float y);

extern struct motzone *nemomotz_one_create(void);
extern int nemomotz_one_prepare(struct motzone *one);
extern void nemomotz_one_finish(struct motzone *one);

extern void nemomotz_one_attach_one(struct motzone *one, struct motzone *child);
extern void nemomotz_one_detach_one(struct motzone *one, struct motzone *child);

extern void nemomotz_one_draw_null(struct nemomotz *motz, struct motzone *one);
extern void nemomotz_one_down_null(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y);
extern void nemomotz_one_motion_null(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y);
extern void nemomotz_one_up_null(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y);
extern struct motzone *nemomotz_one_contain_null(struct motzone *one, float x, float y);
extern void nemomotz_one_update_null(struct motzone *one);
extern int nemomotz_one_frame_null(struct motzone *one, uint32_t msecs);
extern void nemomotz_one_destroy_null(struct motzone *one);

extern struct motztap *nemomotz_tap_create(void);
extern void nemomotz_tap_destroy(struct motztap *tap);

extern void nemomotz_tap_set_id(struct motztap *tap, uint64_t id);
extern void nemomotz_tap_set_one(struct motztap *tap, struct motzone *one);

extern void nemomotz_attach_tap(struct nemomotz *motz, struct motztap *tap);
extern void nemomotz_detach_tap(struct nemomotz *motz, struct motztap *tap);

extern struct motztap *nemomotz_find_tap(struct nemomotz *motz, uint64_t id);

extern void nemomotz_attach_transition(struct nemomotz *motz, struct nemotransition *trans);
extern void nemomotz_detach_transition(struct nemomotz *motz, struct nemotransition *trans);

extern void nemomotz_revoke_transition(struct nemomotz *motz, void *var, int size);

static inline void nemomotz_transform_from_viewport(struct nemomotz *motz, float x, float y, float *sx, float *sy)
{
	if (motz->width == motz->viewport.width && motz->height == motz->viewport.height) {
		*sx = x;
		*sy = y;
	} else {
		*sx = x * (float)motz->width / (float)motz->viewport.width;
		*sy = y * (float)motz->height / (float)motz->viewport.height;
	}
}

static inline void nemomotz_transform_to_viewport(struct nemomotz *motz, float sx, float sy, float *x, float *y)
{
	if (motz->width == motz->viewport.width && motz->height == motz->viewport.height) {
		*x = sx;
		*y = sy;
	} else {
		*x = sx * (float)motz->viewport.width / (float)motz->width;
		*y = sy * (float)motz->viewport.height / (float)motz->height;
	}
}

static inline void nemomotz_set_down_callback(struct nemomotz *motz, nemomotz_down_t down)
{
	if (down != NULL)
		motz->down = down;
	else
		motz->down = nemomotz_down_null;
}

static inline void nemomotz_down(struct nemomotz *motz, struct motztap *tap, float x, float y)
{
	motz->down(motz, tap, x, y);
}

static inline void nemomotz_set_motion_callback(struct nemomotz *motz, nemomotz_motion_t motion)
{
	if (motion != NULL)
		motz->motion = motion;
	else
		motz->motion = nemomotz_motion_null;
}

static inline void nemomotz_motion(struct nemomotz *motz, struct motztap *tap, float x, float y)
{
	motz->motion(motz, tap, x, y);
}

static inline void nemomotz_set_up_callback(struct nemomotz *motz, nemomotz_up_t up)
{
	if (up != NULL)
		motz->up = up;
	else
		motz->up = nemomotz_up_null;
}

static inline void nemomotz_up(struct nemomotz *motz, struct motztap *tap, float x, float y)
{
	motz->up(motz, tap, x, y);
}

static inline void nemomotz_set_flags(struct nemomotz *motz, uint32_t flags)
{
	motz->flags |= flags;
}

static inline void nemomotz_put_flags(struct nemomotz *motz, uint32_t flags)
{
	motz->flags &= ~flags;
}

static inline int nemomotz_has_flags(struct nemomotz *motz, uint32_t flags)
{
	return motz->flags & flags;
}

static inline int nemomotz_has_flags_all(struct nemomotz *motz, uint32_t flags)
{
	return (motz->flags & flags) == flags;
}

static inline void nemomotz_one_set_draw_callback(struct motzone *one, nemomotz_one_draw_t draw)
{
	if (draw != NULL)
		one->draw = draw;
	else
		one->draw = nemomotz_one_draw_null;
}

static inline void nemomotz_one_draw(struct nemomotz *motz, struct motzone *one)
{
	one->draw(motz, one);
}

static inline void nemomotz_one_set_down_callback(struct motzone *one, nemomotz_one_down_t down)
{
	if (down != NULL)
		one->down = down;
	else
		one->down = nemomotz_one_down_null;
}

static inline void nemomotz_one_down(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
	one->down(motz, tap, one, x, y);
}

static inline void nemomotz_one_set_motion_callback(struct motzone *one, nemomotz_one_motion_t motion)
{
	if (motion != NULL)
		one->motion = motion;
	else
		one->motion = nemomotz_one_motion_null;
}

static inline void nemomotz_one_motion(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
	one->motion(motz, tap, one, x, y);
}

static inline void nemomotz_one_set_up_callback(struct motzone *one, nemomotz_one_up_t up)
{
	if (up != NULL)
		one->up = up;
	else
		one->up = nemomotz_one_up_null;
}

static inline void nemomotz_one_up(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
	one->up(motz, tap, one, x, y);
}

static inline void nemomotz_one_set_contain_callback(struct motzone *one, nemomotz_one_contain_t contain)
{
	if (contain != NULL)
		one->contain = contain;
	else
		one->contain = nemomotz_one_contain_null;
}

static inline struct motzone *nemomotz_one_contain(struct motzone *one, float x, float y)
{
	return one->contain(one, x, y);
}

static inline void nemomotz_one_set_update_callback(struct motzone *one, nemomotz_one_update_t update)
{
	if (update != NULL)
		one->update = update;
	else
		one->update = nemomotz_one_update_null;
}

static inline void nemomotz_one_update(struct motzone *one)
{
	one->update(one);

	one->dirty = 0x0;
}

static inline void nemomotz_one_set_frame_callback(struct motzone *one, nemomotz_one_frame_t frame)
{
	if (frame != NULL)
		one->frame = frame;
	else
		one->frame = nemomotz_one_frame_null;
}

static inline int nemomotz_one_frame(struct motzone *one, uint32_t msecs)
{
	return one->frame(one, msecs);
}

static inline void nemomotz_one_set_destroy_callback(struct motzone *one, nemomotz_one_destroy_t destroy)
{
	if (destroy != NULL)
		one->destroy = destroy;
	else
		one->destroy = nemomotz_one_destroy_null;
}

static inline void nemomotz_one_destroy(struct motzone *one)
{
	one->destroy(one);
}

static inline void nemomotz_one_set_dirty(struct motzone *one, uint32_t dirty)
{
	one->dirty |= dirty;
}

static inline void nemomotz_one_put_dirty(struct motzone *one, uint32_t dirty)
{
	one->dirty &= ~dirty;
}

static inline void nemomotz_one_put_dirty_all(struct motzone *one)
{
	one->dirty = 0x0;
}

static inline int nemomotz_one_has_dirty(struct motzone *one, uint32_t dirty)
{
	return one->dirty & dirty;
}

static inline int nemomotz_one_has_dirty_all(struct motzone *one, uint32_t dirty)
{
	return (one->dirty & dirty) == dirty;
}

static inline int nemomotz_one_has_no_dirty(struct motzone *one)
{
	return one->dirty == 0x0;
}

static inline void nemomotz_one_set_flags(struct motzone *one, uint32_t flags)
{
	one->flags |= flags;

	nemomotz_one_set_dirty(one, NEMOMOTZ_ONE_FLAGS_DIRTY);
}

static inline void nemomotz_one_put_flags(struct motzone *one, uint32_t flags)
{
	one->flags &= ~flags;

	nemomotz_one_set_dirty(one, NEMOMOTZ_ONE_FLAGS_DIRTY);
}

static inline int nemomotz_one_has_flags(struct motzone *one, uint32_t flags)
{
	return one->flags & flags;
}

static inline int nemomotz_one_has_flags_all(struct motzone *one, uint32_t flags)
{
	return (one->flags & flags) == flags;
}

static inline void nemomotz_set_userdata(struct nemomotz *motz, void *data)
{
	motz->data = data;
}

static inline void *nemomotz_get_userdata(struct nemomotz *motz)
{
	return motz->data;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
