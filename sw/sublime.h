#ifndef _SUBLIME_H_
#define _SUBLIME_H_
#define SUBLIME_BASE		0x9a000000
#define NUM_VOICES		8
#define WAVETABLE_SIZE		8192

#define VOICE_OSC0_FREQ		0x0
#define VOICE_OSC1_FREQ		0x4
#define VOICE_CTRL		0x8
#define VOICE_ENVELOPE		0xc

#define VOICE_REG(voice, reg)	(((voice & 0x7f) << 4) | reg)

#define LEFT_SAMPLE		0x800
#define RIGHT_SAMPLE		0x804
#define MAIN_CTRL		0x808

#define WAVETABLE0_BASE		(SUBLIME_BASE + 0x10000)
#define WAVETABLE1_BASE		(SUBLIME_BASE + 0x20000)

extern void sublime_init(void);
extern void sublime_set_note(int voice, int osc, uint8_t note, int32_t cents);

#endif
