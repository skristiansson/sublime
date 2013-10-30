#include <stdio.h>
#include <stdint.h>
#include <irq.h>
#include <midi.h>

#define MMIOMIDI_IRQ			31
#define MMIOMIDI_BASE			0xc0000000
#define MMIOMIDI_IN_REG			0x0
#define MMIOMIDI_STATUS_REG		0x4
#define MMIOMIDI_STATUS_FIFO_EMPTY	(1<<0)

static uint32_t mmiomidi_read_reg(uint32_t reg)
{
	return *((uint32_t *)(MMIOMIDI_BASE + reg));
}

static void mmiomidi_isr(void *private_data)
{
	printf("Got midi interrupt!\r\n");
	while (!(mmiomidi_read_reg(MMIOMIDI_STATUS_REG) &
		 MMIOMIDI_STATUS_FIFO_EMPTY)) {
		midi_receive_byte(mmiomidi_read_reg(MMIOMIDI_IN_REG) & 0xff);
	}
}

void mmiomidi_init(void)
{
	irq_install_isr(MMIOMIDI_IRQ, mmiomidi_isr);
	irq_set_mask(irq_get_mask() | (1<<MMIOMIDI_IRQ));
}
