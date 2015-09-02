#ifndef	__NEMOMOTE_RANDOM_EMITTER_H__
#define	__NEMOMOTE_RANDOM_EMITTER_H__

struct nemomote;

struct moterandom {
	double maxrate;
	double minrate;

	double timetonext;
};

extern int nemomote_moterandom_set_property(struct moterandom *emitter, double maxrate, double minrate);

extern int nemomote_moterandom_ready(struct nemomote *mote, struct moterandom *emitter);
extern int nemomote_moterandom_update(struct nemomote *mote, struct moterandom *emitter, double secs);

#endif
