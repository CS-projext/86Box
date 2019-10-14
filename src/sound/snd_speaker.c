#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include "../86box.h"
#include "../timer.h"
#include "../pit.h"
#include "sound.h"
#include "snd_speaker.h"


int speaker_mute = 0, speaker_gated = 0;
int speaker_enable = 0, was_speaker_enable = 0;

int gated,speakval,speakon;


static int32_t speaker_buffer[SOUNDBUFLEN];
static int speaker_pos = 0;


void
speaker_update(void)
{
    int32_t val;

    if (speaker_pos >= sound_pos_global)
	return;

    for (; speaker_pos < sound_pos_global; speaker_pos++) {
	if (speaker_gated && was_speaker_enable) {
		if (!pit.m[2] || pit.m[2]==4)
			val = speakval;
		else if (pit.l[2] < 0x40)
			val = 0xa00;
		else
			val = speakon ? 0x1400 : 0;
	} else
		val = was_speaker_enable ? 0x1400 : 0;

	if (!speaker_enable)
		was_speaker_enable = 0;

	speaker_buffer[speaker_pos] = val;
    }
}


void
speaker_get_buffer(int32_t *buffer, int len, void *p)
{
    int32_t c, val;

    speaker_update();

    if (!speaker_mute) {
	for (c = 0; c < len * 2; c += 2) {
		val = speaker_buffer[c >> 1];
		buffer[c] += val;
		buffer[c + 1] += val;
	}
    }

    speaker_pos = 0;
}


void
speaker_init(void)
{
    sound_add_handler(speaker_get_buffer, NULL);
    speaker_mute = 0;
}
