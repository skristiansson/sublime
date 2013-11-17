#include <stdio.h>
#include <stdint.h>
#include <config.h>
#include <spr-defs.h>
#include <or1k-support.h>
#include <timer.h>

#define TMR_MAX_US		((SPR_TTMR_PERIOD*1e6)/BOARD_CLK_FREQ)

#define MAX_TIMERS		128

#define TMR_RUNNING		(1 << 1)
#define TMR_ACTIVE		(1 << 0)

struct timer {
	int mode;
	uint32_t time_us;
	uint32_t start_time_us;
	uint32_t flags;
	void (*isr_cb)(void *private_data);
	void *private_data;
};

static struct timer timers[MAX_TIMERS];
/* global running flag */
static int ticktimer_running;

static void timer_reset_ticktimer(void)
{
	or1k_mtspr(SPR_TTCR, 0);
}

static void timer_start_ticktimer(uint32_t us)
{
	uint32_t ticks;

	if (us > TMR_MAX_US)
		us = TMR_MAX_US;

	ticks = us*(BOARD_CLK_FREQ/1e6);

	or1k_mtspr(SPR_TTMR, SPR_TTMR_IE | SPR_TTMR_SR | ticks);
}

static uint32_t timer_get_ticktimer_us(void)
{
	return or1k_mfspr(SPR_TTCR)/(BOARD_CLK_FREQ/1e6);
}

static uint32_t timer_get_ticktimer_period_us(void)
{
	return (or1k_mfspr(SPR_TTMR) & SPR_TTMR_PERIOD)/(BOARD_CLK_FREQ/1e6);
}

static void timer_do_timeout(struct timer *timer)
{
	if (timer->mode == TMR_CONTINOUS)
		timer->time_us = timer->start_time_us;
	else if (timer->mode == TMR_ONESHOT)
		timer->flags &= ~TMR_RUNNING;

	if (timer->isr_cb)
		timer->isr_cb(timer->private_data);
}

/*
 * Timer interrupt service routine.
 * Iterates through the timers and runs the callback on the
 * timer(s) that are done.
 * The (hardware) timer is reloaded with the value of the
 * timer that has least time left.
 */
static void timer_isr(void)
{
	int i;
	int next = -1;
	uint32_t elapsed = timer_get_ticktimer_us();
	uint32_t min = 0xffffffff;

	or1k_mtspr(SPR_TTMR, 0);
	timer_reset_ticktimer();
	for (i = 0; i < MAX_TIMERS; i++) {
		if ((timers[i].flags & (TMR_ACTIVE | TMR_RUNNING)) ==
		    (TMR_ACTIVE | TMR_RUNNING)) {
			if (elapsed > timers[i].time_us)
				timers[i].time_us = 0;
			else
				timers[i].time_us -= elapsed;

			if (timers[i].time_us == 0)
				timer_do_timeout(&timers[i]);

			/*
			 * The 'running' flag and time_us might have changed for
			 * a just timed out timer, so we have to recheck them
			 * here.
			 */
			if ((timers[i].flags & TMR_RUNNING) &&
			    (timers[i].time_us < min)) {
				min = timers[i].time_us;
				next = i;
			}
		}
	}

	/* No upcoming active and running timers? */
	if (next < 0) {
		or1k_mtspr(SPR_TTMR, 0);
		ticktimer_running = 0;
		return;
	}

	timer_start_ticktimer(timers[next].time_us);
}

/*
 * Search for a free timer and allocates it.
 * Returns a pointer to the allocated timer.
 */
struct timer *timer_alloc(void (*isr_cb)(void *private_data),
			  void *private_data)
{
	int i = 0;

	for (i = 0; i < MAX_TIMERS; i++) {
		if (!(timers[i].flags & TMR_ACTIVE)) {
			timers[i].flags = TMR_ACTIVE;
			timers[i].isr_cb = isr_cb;
			timers[i].private_data = private_data;
			return &timers[i];
		}
	}

	printf("Error: Could not allocate timer\r\n");
	return 0;
}

void timer_start(struct timer *timer, int mode, uint32_t time_us)
{
	timer->start_time_us = timer->time_us = time_us;
	timer->flags |= TMR_RUNNING;
	/*
	 * A bit tricky, if the timer is running, we have to add the
	 * current timer value to the timer we are inserting.
	 * And if the timer we are inserting should timeout before the
	 * current ongoing timer, the timer period have to be adjusted by
	 * (re)starting the timer with the new timer value.
	 */
	if (ticktimer_running) {
		timer->time_us += timer_get_ticktimer_us();
		if (timer->time_us < timer_get_ticktimer_period_us())
			timer_start_ticktimer(timer->time_us);
	} else {
		timer_reset_ticktimer();
		timer_start_ticktimer(time_us);
		ticktimer_running = 1;
	}

}

void timer_init(void)
{
	int i;

	or1k_exception_handler_add(0x5, timer_isr);

	for (i = 0; i < MAX_TIMERS; i++)
		timers[i].flags = 0;

	/* Enable tick timer exception */
	or1k_mtspr(SPR_SR, or1k_mfspr(SPR_SR) | SPR_SR_TEE);
}
