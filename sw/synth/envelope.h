#ifndef _ENVELOPE_H_
#define _ENVELOPE_H_

/*
 * attack, decay and release values are expressed in micro seconds.
 */
struct envelope {
	int gate;
	int state;
	uint32_t attack;
	uint32_t decay;
	uint8_t sustain;
	uint32_t release;
	uint8_t release_output;
	uint8_t output;
	uint8_t step;
	struct timer *timer;
};

extern int envelope_isactive(struct envelope *envelope);
extern void envelope_gate_on(struct envelope *envelope);
extern void envelope_gate_off(struct envelope *envelope);
extern void envelope_init(struct envelope *envelope);

#endif
