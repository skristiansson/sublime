#include <stdio.h>
#include <stdint.h>
#include <irq.h>
#include <i2c.h>
#include <codec.h>
#include <delay.h>
#include <sublime.h>
#include <midi.h>

static struct sublime sublime_synth;

static void init(void)
{
	irq_init();

#ifdef I2C_DRIVER
	printf("Initializing i2c..");
	i2c_init();
	printf("done\r\n");
#endif
#ifdef CODEC_DRIVER
	printf("Initializing audio codec..");
	codec_init();
	printf("done\r\n");
#endif

	printf("Initializing MIDI..");
	midi_init();
	printf("done\r\n");

	printf("Initializing sublime..");
	sublime_init(&sublime_synth, 8);
	printf("done\r\n");

	irq_enable();
}

int main(void)
{
	init();

	for(;;) {
	}
}
