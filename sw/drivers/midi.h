#ifndef _MIDI_H_
#define _MIDI_H_
#include <stddef.h>
#include <stdint.h>

#if (MIDI_DRIVER == mmiomidi)
#define midi_driver_init mmiomidi_init
#else
#error "Define MIDI_DRIVER"
#endif

#define STATUS_BYTE_MASK	0x80
#define REALTIME_MASK		0xf8
#define NOTE_ON			0x90
#define NOTE_OFF		0x80
#define SYSEX_MSG		0xf0

enum {
	CB_NOTE_ON,
	CB_NOTE_OFF
};

struct midi_msg {
	size_t len;
	uint8_t data[128];
};

struct midi_note {
	uint8_t note;
	uint8_t velocity;
	void *private_data;
};

/* Helper functions for parsing status byte */
static inline int is_note_on(uint8_t status_byte)
{
	return (status_byte & 0xf0) == NOTE_ON;
}

static inline int is_note_off(uint8_t status_byte)
{
	return (status_byte & 0xf0) == NOTE_OFF;

}

static inline uint8_t get_chan(uint8_t status_byte)
{
	return (status_byte >> 4) & 0xf;
}

extern void midi_init(void);
extern void midi_driver_init(void);
extern void midi_receive_byte(uint8_t data);
extern int midi_register_note_on_cb(void *private_data, uint8_t listen_chan,
				    void (*cb)(struct midi_note *midi_note));
extern int midi_register_note_off_cb(void *private_data, uint8_t listen_chan,
				     void (*cb)(struct midi_note *midi_note));

#endif
