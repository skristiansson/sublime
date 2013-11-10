#include <stdint.h>
#include <midi.h>

/* Increase this when needed... */
#define MAX_CALLBACKS	10


struct note_onoff {
	void (*cb[MAX_CALLBACKS])(struct midi_note *midi_note);
	uint8_t listen_chan[MAX_CALLBACKS];
	void *private_data[MAX_CALLBACKS];
	int cb_cnt;
};

static struct note_onoff note_on;
static struct note_onoff note_off;

static struct midi_msg midi_msg;

void midi_handle_note_on_off(struct note_onoff *note_onoff, uint8_t note,
			     uint8_t velocity, uint8_t chan)
{
	int i;
	struct midi_note midi_note;

	for (i = 0; i < note_onoff->cb_cnt; i++) {
		if (note_onoff->cb[i] &&
		    (note_onoff->listen_chan[i] == 0xff ||
		     note_onoff->listen_chan[i] == chan)) {
			midi_note.note = note;
			midi_note.velocity = velocity;
			midi_note.private_data = note_onoff->private_data[i];
			note_onoff->cb[i](&midi_note);
		}
	}
}

void midi_handle_msg(struct midi_msg *msg)
{
	uint8_t status_byte = msg->data[0];

	switch (status_byte & 0xf0) {
	case NOTE_ON:
		/* Treat note on with 0 velocity as note off */
		if (msg->data[2] == 0) {
			msg->data[0] = (status_byte & 0x0f) | NOTE_OFF;
			midi_handle_msg(msg);
			break;
		}

		midi_handle_note_on_off(&note_on, msg->data[1],	msg->data[2],
					get_chan(status_byte));
		break;

	case NOTE_OFF:
		midi_handle_note_on_off(&note_off, msg->data[1], msg->data[2],
					get_chan(status_byte));
		break;

	case PITCH_WHEEL_CHANGE:
		break;

	default:
		break;
	}
}

/*
 * Receives one byte in the midi stream and organizes it into
 * the current message.
 */
void midi_receive_byte(uint8_t data)
{
	static int pos = 0;
	static uint8_t status_byte = 0;
	int msg_done = 0;

	struct midi_msg *msg = &midi_msg;

	msg->data[0] = status_byte;
	if (data & STATUS_BYTE_MASK) {
		msg->data[0] = status_byte = data;
		pos = 1;
		if ((data & REALTIME_MASK) == REALTIME_MASK || data > 0xf3)
			msg_done = 1;
	} else {
		msg->data[pos++] = data;
		if ((pos > 2) || (status_byte > 0xbf && status_byte < 0xe0) ||
		    (status_byte == 0xf3))
			msg_done = 1;
	}

	msg->len = pos;
	if (msg_done) {
		pos = 1;
		/*
		 * TODO: create msg queue and move this out of interrupt context
		 */
		midi_handle_msg(msg);
	}
}

int midi_register_note_on_cb(void *private_data, uint8_t listen_chan,
			     void (*cb)(struct midi_note *midi_note))
{
	if (note_on.cb_cnt >= MAX_CALLBACKS)
		return -1;

	note_on.cb[note_on.cb_cnt] = cb;
	note_on.private_data[note_on.cb_cnt] = private_data;
	note_on.listen_chan[note_on.cb_cnt] = listen_chan;
	note_on.cb_cnt++;

	return 0;
}

int midi_register_note_off_cb(void *private_data, uint8_t listen_chan,
			      void (*cb)(struct midi_note *midi_note))
{
	if (note_off.cb_cnt >= MAX_CALLBACKS)
		return -1;

	note_off.cb[note_off.cb_cnt] = cb;
	note_off.private_data[note_off.cb_cnt] = private_data;
	note_off.listen_chan[note_off.cb_cnt] = listen_chan;
	note_off.cb_cnt++;

	return 0;
}

void midi_init(void)
{
	midi_driver_init();
}
