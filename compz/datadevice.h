#ifndef	__NEMO_DATADEVICE_H__
#define	__NEMO_DATADEVICE_H__

struct nemoseat;
struct nemoview;

struct nemodatasource {
	struct wl_resource *resource;
	struct wl_signal destroy_signal;
	struct wl_array mime_types;

	void (*accept)(struct nemodatasource *source, uint32_t serial, const char *mime_type);
	void (*send)(struct nemodatasource *source, const char *mime_type, int32_t fd);
	void (*cancel)(struct nemodatasource *source);
};

extern int datadevice_manager_init(struct wl_display *display);

extern void datadevice_set_focus(struct nemoseat *seat, struct nemoview *view);

#endif
