#ifndef _TIMER_H_
#define _TIMER_H_
enum {
	TMR_ONESHOT,
	TMR_CONTINOUS,
};

struct timer;

extern struct timer *timer_alloc(void (*isr_cb)(void *private_data),
				 void *private_data);
extern void timer_start(struct timer *timer, int mode, uint32_t time_us);
extern void timer_init(void);
#endif
