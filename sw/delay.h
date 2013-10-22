#ifndef _DELAY_H_
#define _DELAY_H_
#include <config.h>
/*
 * Extremely inaccurate delay function, to be replaced
 * with a timer driven solution later.
 */
static void delay_ms(uint32_t ms)
{
	volatile uint32_t ticks = (BOARD_CLK_FREQ/1000)*ms;

	while (ticks--)
		;
}
#endif
