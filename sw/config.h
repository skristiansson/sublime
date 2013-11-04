#ifndef _CONFIG_H_
#define _CONFIG_H_
/* Board config */
#define BOARD_MEM_BASE		0x00000000
#define BOARD_MEM_SIZE		0x40000000 /* 1GB */
#define BOARD_CLK_FREQ		50e6
#define BOARD_UART_BASE		0x90000000
#define BOARD_UART_BAUD		115200
#define BOARD_UART_IRQ		2

#define BOARD_SUBLIME_BASE	0x9a000000

/* Driver config */
#define I2C_DRIVER		oci2c
#define CODEC_DRIVER		ssm2603
#define MIDI_DRIVER		mmiomidi
#endif
