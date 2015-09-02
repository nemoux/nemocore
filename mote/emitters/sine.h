#ifndef	__NEMOMOTE_SINE_EMITTER_H__
#define	__NEMOMOTE_SINE_EMITTER_H__

struct nemomote;

struct motesine {
	double maxrate;
	double minrate;
	double period;
	double factor;
	double scale;

	double timepassed;
};

extern int nemomote_sine_set_property(struct motesine *emitter, double maxrate, double minrate, double period);

extern int nemomote_sine_ready(struct nemomote *mote, struct motesine *emitter);
extern int nemomote_sine_update(struct nemomote *mote, struct motesine *emitter, double secs);

#endif
