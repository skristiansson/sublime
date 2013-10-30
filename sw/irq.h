#ifndef _IRQ_H_
#define _IRQ_H_
#include <spr-defs.h>
#include <or1k-support.h>

extern void irq_init(void);

static void irq_install_isr(int irq, void(*fn)(void*))
{
	or1k_interrupt_handler_add(irq, fn);
}

static void irq_enable(void)
{
	or1k_mtspr(SPR_SR, or1k_mfspr(SPR_SR) | SPR_SR_IEE);
}

static void irq_disable(void)
{
	or1k_mtspr(SPR_SR, or1k_mfspr(SPR_SR) & ~SPR_SR_IEE);
}

static unsigned long irq_get_mask(void)
{
	return or1k_mfspr(SPR_PICMR);
}

static void irq_set_mask(unsigned long mask)
{
	or1k_mtspr(SPR_PICMR, mask);
}

#endif
