#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>

#define HAVE_STDARG_H
#include "../plat.h"
#include "../plat_midi.h"
#include "../game/gameport.h"
#include "../mouse.h"
#include "../ui.h"

int ui_msgbox(int type, void* arg) {
  printf("ui_msgbox stub: type=%d, arg=%s", type, (char*)arg);
  return 0;
}

joystick_t joystick_state[MAX_JOYSTICKS];
void joystick_init(void) { return; }
void joystick_process(void) { return; }
void plat_cdrom_ui_update(uint8_t a, uint8_t b) { return; }
wchar_t* plat_get_string(int a) { return NULL; }
void plat_midi_close(void) { return; }
int plat_midi_get_num_devs(void) { return 0; }
void plat_midi_init(void) { return; }
void plat_midi_play_msg(uint8_t* a) { return; }
void plat_midi_play_sysex(uint8_t* a, unsigned int b) { return; }
int plat_midi_write(uint8_t a) { return 0; }
void plat_pause(int a) { return; }
void ui_sb_bugui(char* a) { return; }
void ui_sb_set_ready(int a) { return; }
void ui_sb_update_icon(int a, int b) { return; }
void ui_sb_update_icon_state(int a, int b) { return; }
void ui_sb_update_panes(void) { return; }
void zip_eject(uint8_t a) { return; }
void zip_reload(uint8_t a) { return; }
