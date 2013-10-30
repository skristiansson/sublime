#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <config.h>
#include <sublime.h>

static int32_t* wavetable0;
static int32_t* wavetable1;

static uint32_t note_table[129];
static uint32_t cent_table[101];

struct voice {
	uint8_t note;
	uint8_t cents;
	uint8_t velocity;
};

void sublime_write_reg(struct sublime *sublime, uint32_t reg, uint32_t value)
{
	*((uint32_t *)(sublime->base + reg)) = value;
}

uint32_t sublime_read_reg(struct sublime *sublime, uint32_t reg)
{
	return *((uint32_t *)(sublime->base + reg));
}

static void gen_triangle(int32_t *dest)
{
	int32_t i;

	for (i = 0; i < WAVETABLE_SIZE/4; i++)
		*dest++ = i*(INT_MAX/(WAVETABLE_SIZE/4));

	for (i = 0; i < WAVETABLE_SIZE/4; i++)
		*dest++ = INT_MAX - i*(INT_MAX/(WAVETABLE_SIZE/4));

	for (i = 0; i < WAVETABLE_SIZE/4; i++)
		*dest++ = -i*(INT_MAX/(WAVETABLE_SIZE/4));

	for (i = 0; i < WAVETABLE_SIZE/4; i++)
		*dest++ = i*(INT_MAX/(WAVETABLE_SIZE/4)) - INT_MAX;
}

static void gen_saw(int32_t *dest)
{
	int32_t i;

	for (i = 0; i < WAVETABLE_SIZE; i++)
		*dest++ = INT_MAX - i*(INT_MAX/(WAVETABLE_SIZE/2));

}

static void gen_square(int32_t *dest)
{
	int32_t i;

	for (i = 0; i < WAVETABLE_SIZE/2; i++)
		*dest++ = INT_MAX-1;

	for (i = 0; i < WAVETABLE_SIZE/2; i++)
		*dest++ = -(INT_MAX-1);
}

static void gen_note_table(void)
{
	double notes[129];
	int i;

	notes[69] = 440; /* #A4 */
	for (i = 70; i <= 80; i++)
		notes[i] = notes[i-1] * pow(2, 1.0f/12);

	for (i = 68; i >= 0; i--)
		notes[i] = notes[i+12]/2;

	for(i = 81; i <= 128; i++)
		notes[i] = notes[i-12]*2;

	/*
	 * The nco phase acc is 32-bit, so the formula for the value to
	 * to write into the freq reg is:
	 * 2^32/(BOARD_CLK_FREQ/note_freq)
	 */
	for (i = 0; i < 128; i++) {
		note_table[i] = ((1ull<<32)*(uint64_t)notes[i])/BOARD_CLK_FREQ;
		printf("SJK DEBUG: note_table[%i] = %x, notes[%i] = %f\r\n",
		       i, note_table[i], i, notes[i]);
	}
}

static void gen_cent_table(void)
{
	int i;

	for (i = 0; i <= 100; i++)
		cent_table[i] = (1<<16) / pow(2, (float)i/1200) + 0.5f;
}

int32_t sublime_read_left(void)
{
	return *((int32_t *)(SUBLIME_BASE + LEFT_SAMPLE));
}

/*
 * Converts note and cents values into 'freq' value suitable for sublime's
 * freq registers (i.e. the phase accumulator increamental value)
 */
uint32_t sublime_get_freq(int8_t note, int32_t cents)
{
	uint64_t freq_val;

	/*
	 * adjust cents to be between 0 and 99,
	 * increase/decrease notes accordingly
	 */
	if (cents > 100) {
		note += cents/100;
		cents %= 100;
	} else if (cents < 0) {
		note += (cents-100)/100;
		cents = 100 + cents%100;
	}

	if (note < 0)
		note = 0;

	freq_val = ((uint64_t)note_table[note+1] *
		    (uint64_t)cent_table[100-cents]) >> 16;

	return (uint32_t) freq_val;
}

void sublime_set_note(struct sublime *sublime, int voice, int osc,
		      int8_t note, int32_t cents)
{
	uint32_t freq_val = sublime_get_freq(note, cents);

	if (sublime->voices[voice].active) {
		printf("SJK DEBUG: set_note: note = %i, cents = %i, "
		       "freq_val = %x, velocity = %x\r\n",
		       note, cents, freq_val, sublime->voices[voice].velocity);
	}

	osc = (osc == 0) ? VOICE_OSC0_FREQ : VOICE_OSC1_FREQ;
	sublime_write_reg(sublime, VOICE_REG(voice, osc), freq_val);
}

void sublime_init(struct sublime *sublime, int num_voices)
{
	int i;

	/* Generate the lookup tables */
	gen_note_table();
	gen_cent_table();

	sublime->base = SUBLIME_BASE; /* SJK MOVE */
	sublime->num_voices = num_voices;

	for (i = 0; i < num_voices; i++) {
		sublime->voices[i].active = 0;
		sublime->voices[i].osc[0].enable = 1;
	}

	/* Reset all voice registers */
	for (i = 0; i < 4*num_voices; i++)
		sublime_write_reg(sublime, SUBLIME_BASE + i*4, 0);

	/* Set defaults */
	wavetable0 = (int32_t *)WAVETABLE0_BASE;
	wavetable1 = (int32_t *)WAVETABLE1_BASE;
	gen_saw(wavetable0);
	gen_square(wavetable1);

	/* Assert sync to all voices */
	sublime_write_reg(sublime, MAIN_CTRL, 1);

	/* Deassert sync to all voices */
	sublime_write_reg(sublime, MAIN_CTRL, 0);

}
