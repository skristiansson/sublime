#include <stdio.h>
#include <irq.h>

extern void or1k_interrupt_handler(void);

void unhandled_interrupt(void *privdata)
{
	printf("Unhandled interrupt!\r\n");
	printf("HANG\r\n");
	for(;;) {
		;
	}
}

void irq_init(void)
{
	int i;

	/* Mask all interrupts */
	irq_set_mask(0);
	irq_disable();
	or1k_exception_handler_add(0x8, or1k_interrupt_handler);
	for (i = 0; i < 32; i++) {
		or1k_interrupt_handler_add(i, unhandled_interrupt);
	}
}
