#include <stdio.h>
#include <stdint.h>
#include <i2c.h>
#include <codec.h>
#include <delay.h>
static void init(void)
{
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

	printf("Initializing sublime..");
	sublime_init();
	printf("done\r\n");

}

int main(void)
{
	volatile i;
	int32_t left_sample;
	uint8_t j = 0;
	init();


	for(;;) {
		delay_ms(10);
		sublime_set_note(0, 0, j++%128, 0);
		left_sample = sublime_read_left();
		printf("left_sample = %i\t0x%08x\r\n",
		       left_sample, left_sample);


	}
}
