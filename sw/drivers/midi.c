#include <stdint.h>
#include <midi.h>

/* Increase this when needed... */
#define MAX_CALLBACKS	10


struct midi_event {
	void (*cb[MAX_CALLBACKS])(struct midi *midi);
	uint8_t listen_chan[MAX_CALLBACKS];
	void *private_data[MAX_CALLBACKS];
	int cb_cnt;
};

static struct midi_event midi_events[MIDI_EVENT_MAX];
static struct midi_msg midi_msg;

static void midi_handle_event(struct midi_event *event, struct midi *midi)
{
	int i;

	for (i = 0; i < event->cb_cnt; i++) {
		if (event->cb[i] && (event->listen_chan[i] == 0xff ||
				     event->listen_chan[i] == midi->chan)) {
			midi->private_data = event->private_data[i];
			event->cb[i](midi);
		}
	}
}

/*
 * Parse the midi msg type
 */
void midi_handle_msg(struct midi_msg *msg)
{
	struct midi midi;
	uint8_t status_byte = msg->data[0];

	midi.chan = get_chan(status_byte);

	switch (status_byte & 0xf0) {
	case NOTE_ON:
		/* Treat note on with 0 velocity as note off */
		if (msg->data[2] == 0) {
			msg->data[0] = (status_byte & 0x0f) | NOTE_OFF;
			midi_handle_msg(msg);
			break;
		}

		midi.note.key = msg->data[1];
		midi.note.velocity = msg->data[2];
		midi_handle_event(&midi_events[MIDI_EVENT_NOTE_ON], &midi);
		break;

	case NOTE_OFF:
		midi.note.key = msg->data[1];
		midi.note.velocity = msg->data[2];
		midi_handle_event(&midi_events[MIDI_EVENT_NOTE_OFF], &midi);
		break;

	case PITCHWHEEL_CHANGE:
		/*
		 * pitchwheel change data is a 14 bit unsigned value with 0x2000
		 * representing 0
		 */
		midi.pitchwheel = ((msg->data[2] << 7) | msg->data[1]) - 0x2000;
		midi_handle_event(&midi_events[MIDI_EVENT_PW_CHANGE], &midi);
		break;

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

/*
 * Register a callback for a midi event.
 * Returns 0 on success.
 */
int midi_register_cb(int event_type, void *private_data, uint8_t listen_chan,
		     void (*cb)(struct midi *midi))
{
	struct midi_event *event;

	if (event_type > MIDI_EVENT_MAX || event_type < 0)
		return -1;

	event = &midi_events[event_type];
	if (event->cb_cnt >= MAX_CALLBACKS)
		return -2;

	event->cb[event->cb_cnt] = cb;
	event->private_data[event->cb_cnt] = private_data;
	event->listen_chan[event->cb_cnt] = listen_chan;
	event->cb_cnt++;

	return 0;
}

void midi_init(void)
{
	midi_driver_init();
}
