#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <config.h>
#include <midi.h>
#include <sublime.h>

static uint32_t note_table[129];
static uint32_t cent_table[101];

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
		*dest++ = i*(INT_MAX/WAVETABLE_SIZE);

	for (i = 0; i < WAVETABLE_SIZE/4; i++)
		*dest++ = INT_MAX/4 - i*(INT_MAX/WAVETABLE_SIZE);

	for (i = 0; i < WAVETABLE_SIZE/4; i++)
		*dest++ = -i*(INT_MAX/WAVETABLE_SIZE);

	for (i = 0; i < WAVETABLE_SIZE/4; i++)
		*dest++ = i*(INT_MAX/WAVETABLE_SIZE) - INT_MAX;
}

static void gen_saw(int32_t *dest)
{
	int32_t i;

	for (i = 0; i < WAVETABLE_SIZE; i++)
		*dest++ = INT_MAX/4 - i*(INT_MAX/(WAVETABLE_SIZE*2));

}

static void gen_square(int32_t *dest)
{
	int32_t i;

	for (i = 0; i < WAVETABLE_SIZE/2; i++)
		*dest++ = INT_MAX/4;

	for (i = 0; i < WAVETABLE_SIZE/2; i++)
		*dest++ = -(INT_MAX/4);
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
	for (i = 0; i <= 128; i++)
		note_table[i] = ((1ull<<32)*notes[i])/BOARD_CLK_FREQ + 0.5f;
}

static void gen_cent_table(void)
{
	int i;

	for (i = 0; i <= 100; i++)
		cent_table[i] = (1<<16) / pow(2, (float)i/1200) + 0.5f;
}

int32_t sublime_read_left(struct sublime *sublime)
{
	return *((int32_t *)(sublime->base + LEFT_SAMPLE));
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

	osc = (osc == 0) ? VOICE_OSC0_FREQ : VOICE_OSC1_FREQ;
	sublime_write_reg(sublime, VOICE_REG(voice, osc), freq_val);
}

int sublime_get_free_voice(struct sublime *sublime)
{
	for (int i = 0; i < sublime->num_voices; i++) {
		if (!sublime->voices[i].active)
			return i;
	}

	return -1;
}

int sublime_get_voice_by_note(struct sublime *sublime, uint8_t note)
{
	for (int i = 0; i < sublime->num_voices; i++) {
		if (sublime->voices[i].active &&
		    sublime->voices[i].note == note)
			return i;
	}

	return -1;
}

void sublime_note_on_cb(struct midi_note *midi_note)
{
	struct sublime *sublime = midi_note->private_data;
	int voice;

	voice = sublime_get_free_voice(sublime);
	if (voice < 0)
		return;

	sublime->voices[voice].note = midi_note->note;
	sublime->voices[voice].velocity = midi_note->velocity;
	sublime->voices[voice].active = 1;
}

void sublime_note_off_cb(struct midi_note *midi_note)
{
	struct sublime *sublime = midi_note->private_data;
	int voice;

	voice = sublime_get_voice_by_note(sublime, midi_note->note);
	if (voice < 0)
		return;

	sublime->voices[voice].note = 0;
	sublime->voices[voice].velocity = 0;
	sublime->voices[voice].active = 0;
}

void sublime_write_voice(struct sublime *sublime, int voice_idx)
{
	struct voice *voice = &sublime->voices[voice_idx];

	sublime_write_reg(sublime, VOICE_REG(voice_idx, VOICE_CTRL),
			  (uint16_t)voice->velocity << 8 |
			  voice->osc[1].enable << 1 | voice->osc[0].enable);
	sublime_set_note(sublime, voice_idx, 0, voice->note, 0);
	sublime_set_note(sublime, voice_idx, 1, voice->note, 0);
}

void sublime_task(struct sublime *sublime)
{
	for (int i = 0; i < sublime->num_voices; i++) {
		sublime_write_voice(sublime, i);
	}
}

void sublime_init(struct sublime *sublime, void *base)
{
	int i;

	/* Generate the lookup tables */
	gen_note_table();
	gen_cent_table();

	sublime->base = base;
	sublime->num_voices = sublime_read_reg(sublime, SUBLIME_CONFIG) & 0x7f;
	printf("SJK DEBUG: sublime->num_voices = %d\r\n", sublime->num_voices);

	for (i = 0; i < sublime->num_voices; i++) {
		sublime->voices[i].active = 0;
		sublime->voices[i].osc[0].enable = 1;
		sublime->voices[i].osc[1].enable = 1;
	}

	/* Reset all voice registers */
	for (i = 0; i < 4*sublime->num_voices; i++)
		sublime_write_reg(sublime, i*4, 0);

	/* Set defaults */
	sublime->wavetable0 = (int32_t *)(base + WAVETABLE0);
	sublime->wavetable1 = (int32_t *)(base + WAVETABLE1);
	gen_saw(sublime->wavetable0);
	gen_square(sublime->wavetable1);

	/* Assert sync to all voices */
	sublime_write_reg(sublime, MAIN_CTRL, 1);

	/* Deassert sync to all voices */
	sublime_write_reg(sublime, MAIN_CTRL, 0);

	midi_register_note_on_cb(sublime, 0xff, sublime_note_on_cb);
	midi_register_note_off_cb(sublime, 0xff, sublime_note_off_cb);
}
