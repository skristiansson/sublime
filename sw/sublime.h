#ifndef _SUBLIME_H_
#define _SUBLIME_H_
#define SUBLIME_BASE		0x9a000000
#define WAVETABLE_SIZE		8192
#define MAX_NUM_VOICES		127

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

struct osc {
	int enable;
	int8_t cents;
};

struct voice {
	int gate;
	int active;
	uint8_t attack;
	uint8_t decay;
	uint8_t sustain;
	uint8_t release;
	uint8_t velocity;
	uint8_t note;
	struct osc osc[2];
};

struct sublime {
	void *base;
	int num_voices;
	int32_t *wavetable0;
	int32_t *wavetable1;
	struct voice voices[MAX_NUM_VOICES];
};

extern void sublime_init(struct sublime *sublime, int num_voices);
extern void sublime_task(struct sublime *sublime);

#endif
