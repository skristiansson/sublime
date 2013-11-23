/*
 * Each envelope stage (except sustain) set up a timer that trigger an interrupt
 * when it is time to increase/decrease the output.
 * The increase/decrease steps are limited so that the time between interrupts
 * is always greater or equal to 1 ms (1 kHz).
 */
#include <stdint.h>
#include <timer.h>
#include <envelope.h>

enum {
	ENVELOPE_IDLE,
	ENVELOPE_ATTACK,
	ENVELOPE_DECAY,
	ENVELOPE_SUSTAIN,
	ENVELOPE_RELEASE,
};

/* Helper function to limit interrupt rate to 1kHz */
static uint8_t get_step(uint32_t wait_us)
{
	uint32_t step;

	if (wait_us >= 1000)
		return 1;

	if (wait_us == 0)
		return 0xff;

	step = 1000/wait_us;

	return step > 0xff ? 0xff : step;
}

static int do_release(struct envelope *envelope)
{
	uint32_t wait_us;

	if (envelope->output == 0)
		return ENVELOPE_IDLE;

	/*
	 * Scale the release time according to the output value when
	 * release was initiated
	 */
	wait_us = envelope->release / envelope->release_output;

	envelope->step = get_step(wait_us);
	wait_us *= envelope->step;

	if (envelope->step > envelope->output)
		envelope->output = 0;
	else
		envelope->output -= envelope->step;

	timer_start(envelope->timer, TMR_ONESHOT, wait_us);

	return ENVELOPE_RELEASE;
}

static int do_decay(struct envelope *envelope)
{
	uint32_t wait_us;

	envelope->release_output = envelope->output;
	if (!envelope->gate)
		return do_release(envelope);

	if (envelope->output == envelope->sustain)
		return ENVELOPE_SUSTAIN;

	wait_us = envelope->decay / (0xff - envelope->sustain);
	envelope->step = get_step(wait_us);
	wait_us *= envelope->step;

	if (envelope->step > (envelope->output - envelope->sustain))
		envelope->output = envelope->sustain;
	else
		envelope->output -= envelope->step;

	timer_start(envelope->timer, TMR_ONESHOT, wait_us);

	return ENVELOPE_DECAY;
}

static int do_attack(struct envelope *envelope)
{
	uint32_t wait_us;

	envelope->release_output = envelope->output;
	if (!envelope->gate)
		return do_release(envelope);

	/* Attack phase done */
	if (envelope->output == 0xff)
		return do_decay(envelope);

	wait_us = envelope->attack/256;
	envelope->step = get_step(wait_us);
	wait_us *= envelope->step;

	if (envelope->step > (0xff - envelope->output))
		envelope->output = 0xff;
	else
		envelope->output += envelope->step;

	timer_start(envelope->timer, TMR_ONESHOT, wait_us);

	return ENVELOPE_ATTACK;
}

static void envelope_timer_isr(void *private_data)
{
	struct envelope *envelope = private_data;

	switch (envelope->state) {
	case ENVELOPE_ATTACK:
		envelope->state = do_attack(envelope);
		break;

	case ENVELOPE_DECAY:
		envelope->state = do_decay(envelope);
		break;

	case ENVELOPE_RELEASE:
		envelope->state = do_release(envelope);
		break;

	default:
	case ENVELOPE_SUSTAIN:
	case ENVELOPE_IDLE:
		break;
	}
}

int envelope_isactive(struct envelope *envelope)
{
	return envelope->state != ENVELOPE_IDLE;
}

void envelope_gate_on(struct envelope *envelope)
{
	uint32_t wait_us;

	envelope->gate = 1;
	envelope->state = ENVELOPE_ATTACK;
	envelope->output = 0;
	wait_us = envelope->attack/256;
	timer_start(envelope->timer, TMR_ONESHOT, wait_us*get_step(wait_us));
}

void envelope_gate_off(struct envelope *envelope)
{
	envelope->gate = 0;
	/* Gate off will be handled in all the other cases by the timer isr */
	if (envelope->state == ENVELOPE_SUSTAIN)
		envelope->state	= do_release(envelope);
}

void envelope_init(struct envelope *envelope)
{
	envelope->gate = 0;
	envelope->timer = timer_alloc(envelope_timer_isr, envelope);
}
