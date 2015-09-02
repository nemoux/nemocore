#ifndef	__NEMOMOTE_BLAST_EMITTER_H__
#define	__NEMOMOTE_BLAST_EMITTER_H__

struct nemomote;

struct moteblast {
	unsigned int startcount;
};

extern int nemomote_moteblast_set_property(struct moteblast *emitter, unsigned int startcount);

extern int nemomote_moteblast_ready(struct nemomote *mote, struct moteblast *emitter);
extern int nemomote_moteblast_update(struct nemomote *mote, struct moteblast *emitter, double secs);

#endif
