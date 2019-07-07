#include <SDL2/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#undef HAVE_STDARG_H
#define HAVE_STDARG_H
#include "../86box.h"
#include "../plat.h"
#include "../config.h"
#include "../device.h"
#include "../keyboard.h"
#include "../mouse.h"
#include "../floppy/fdd.h"
#include "../cdrom/cdrom.h"
#include "../video/video.h"

#include "linux.h"
#include "linux_sdl.h"

static SDL_Window* sdl_win = NULL;
static SDL_Renderer* sdl_render = NULL;
static SDL_Texture* sdl_tex = NULL;
static SDL_Event event;
static int sdl_fs;
static int cur_w, cur_h;
static struct {
  uint32_t dx;
  uint32_t dy;
  uint32_t dwheel;
  uint8_t buttons;
} sdl_mousedata;

#ifdef ENABLE_SDL_LOG
int sdl_do_log = ENABLE_SDL_LOG;

void sdl_log(const char* fmt, ...) {
  va_list ap;
  if(sdl_do_log) {
    va_start(ap, fmt);
    pclog_ex(fmt, ap);
    va_end(ap);
  }
}
#else
#define sdl_log(fmt, ...)
#endif

static void sdl_blit(int x, int y, int y1, int y2, int w, int h) {
  SDL_Rect r_src;
  void* pixeldata;
  int pitch;
  int yy, ret;

  if(y1 == y2) {
    video_blit_complete();
    return;
  }

  if(buffer32 == NULL) {
    video_blit_complete();
    return;
  }

  SDL_LockTexture(sdl_tex, NULL, &pixeldata, &pitch);

  for(yy = y1; yy < y2; ++yy) {
    if ((y + yy) >= 0 && (y + yy) < buffer32->h) {
      if (video_grayscale || invert_display) {
        video_transform_copy((uint32_t*) &(((uint8_t*)pixeldata)[yy * pitch]),
          &(((uint32_t*)buffer32->line[y + yy])[x]), w);
      } else {
        memcpy((uint32_t*) &(((uint8_t*)pixeldata)[yy * pitch]),
          &(((uint32_t*)buffer32->line[y + yy])[x]), w * 4);
      }
    }
  }

  video_blit_complete();
  SDL_UnlockTexture(sdl_tex);

  r_src.x = 0;
  r_src.y = 0;
  r_src.w = w;
  r_src.h = h;

  ret = SDL_RenderCopy(sdl_render, sdl_tex, &r_src, NULL);
  if(ret) {
    sdl_log("SDL: unable to copy texture to renderer (%s)\n", SDL_GetError());
  }

  SDL_RenderPresent(sdl_render);
}

void sdl_close(void) {
  video_setblit(NULL);

  if(sdl_tex != NULL) {
    SDL_DestroyTexture(sdl_tex);
    sdl_tex = NULL;
  }

  if (sdl_render != NULL) {
    SDL_DestroyRenderer(sdl_render);
    sdl_render = NULL;
  }

  if (sdl_win != NULL) {
    SDL_DestroyWindow(sdl_win);
    sdl_win = NULL;
  }

  SDL_Quit();
}

static int sdl_init_common(int fs) {
  char temp[128];
  SDL_version ver;

  sdl_log("SDL: init (fs=%d)\n", fs);

  cgapal_rebuild();

  SDL_GetVersion(&ver);
  sdl_log("SDL: version %d.%d.%d\n", ver.major, ver.minor, ver.patch);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    sdl_log("SDL: initialization failed (%s)\n", SDL_GetError());
    return 0;
  }

  sprintf(temp, "%s v%s", EMU_NAME, EMU_VERSION);
  sdl_win = SDL_CreateWindow(temp,
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    scrnsz_x, scrnsz_y, fs ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  mouse_capture = 0;
  if(sdl_win == NULL) {
    sdl_log("SDL: unable to create window (%s)\n", SDL_GetError());
    sdl_close();
    return 0;
  }

  sdl_render = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED);
  if(sdl_render == NULL) {
    sdl_log("SDL: unable to create renderer (%s)\n", SDL_GetError());
    sdl_close();
    return 0;
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

  sdl_tex = SDL_CreateTexture(sdl_render, SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING, 2048, 2048);
  if(sdl_tex == NULL) {
    sdl_log("SDL: unable to create texture (%s)\n", SDL_GetError());
    sdl_close();
    return 0;
  }

  /* Make sure we get a clean exit. */
  atexit(sdl_close);

  /* Register our renderer! */
  video_setblit(sdl_blit);

  sdl_fs = fs;

  return 1;
}

int sdl_init(void) {
  return sdl_init_common(0);
}

int sdl_init_fs(void) {
  return sdl_init_common(1);
}

int sdl_pause(void) {
  return 0;
}

void sdl_resize(int x, int y) {
  if((x == cur_w) && (y == cur_h)) return;

  sdl_log("SDL: resize (%i, %i)\n", x, y);
  SDL_SetWindowSize(sdl_win, x, y);
  /* TODO: fs mode scaling? */

  cur_w = x;
  cur_h = y;
}

void plat_resize(int x, int y) {
  if(!vid_resize) video_wait_for_blit();
  sdl_resize(x, y);
}

int sdl_keyboard_handler(SDL_KeyboardEvent* event) {
  uint16_t scancode;
  switch(event->keysym.scancode) {
    case SDL_SCANCODE_LCTRL:
      scancode = 0x01D;
      break;
    case SDL_SCANCODE_LSHIFT:
      scancode = 0x02A;
      break;
    case SDL_SCANCODE_LALT:
      scancode = 0x038;
      break;
    case SDL_SCANCODE_LGUI:
      scancode = 0x15B;
      break;
    case SDL_SCANCODE_RCTRL:
      scancode = rctrl_is_lalt ? 0x038 : 0x11D;
      break;
    case SDL_SCANCODE_RSHIFT:
      scancode = 0x036;
      break;
    case SDL_SCANCODE_RALT:
      scancode = 0x138;
      break;
    case SDL_SCANCODE_RGUI:
      scancode = 0x15C;
      break;
    default:
      if (event->keysym.scancode < SDL_SCANMAP_LEN) {
        scancode = sdl_scanmap[event->keysym.scancode];
      } else {
        /* We found a key we won't process.. */
        return 0;
      }
      break;
  }
  /* 0xFFFF is a sentinel for invalid keys */
  if(scancode != 0xFFFF) {
    keyboard_input(event->type == SDL_KEYDOWN ? 1 : 0, scancode);
    return 1;
  }

  return 0;
}

void sdl_mousemotion_handler(SDL_MouseMotionEvent* event) {
  sdl_mousedata.dx = event->xrel;
  sdl_mousedata.dy = event->yrel;
}

void sdl_mousebutton_handler(SDL_MouseButtonEvent* event) {
  if (event->type == SDL_MOUSEBUTTONDOWN) {
    if(event->button == SDL_BUTTON_LEFT) {
      sdl_mousedata.buttons |= 1;
    }
    if(event->button == SDL_BUTTON_RIGHT) {
      sdl_mousedata.buttons |= 2;
    }
    if(event->button == SDL_BUTTON_MIDDLE) {
      sdl_mousedata.buttons |= 4;
    }
  } else {
    if(event->button == SDL_BUTTON_LEFT) {
      sdl_mousedata.buttons &= ~1;
    }
    if(event->button == SDL_BUTTON_RIGHT) {
      sdl_mousedata.buttons &= ~2;
    }
    if(event->button == SDL_BUTTON_MIDDLE) {
      sdl_mousedata.buttons &= ~4;
    }
  }
}

void sdl_mousewheel_handler(SDL_MouseWheelEvent* event) {
  sdl_mousedata.dwheel = event->y;
}

void sdl_eventpump(void) {
  while(SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      quited = 1;
    }
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      if(event.type == SDL_KEYDOWN && event.key.keysym.mod == KMOD_RCTRL) {
        if(event.key.keysym.sym == SDLK_r) {
          pc_reset(0);
          continue;
        }
        /* TODO: temporary kludges in lieu of a proper UI */
        if(event.key.keysym.sym == SDLK_a || event.key.keysym.sym == SDLK_s) {
          int add_sub = event.key.keysym.sym == SDLK_a ? -1 : 1;
          scale = (scale + add_sub) % 4;
          device_force_redraw();
  				video_force_resize_set(1);
          config_save();
          continue;
        }
        if(event.key.keysym.sym == SDLK_z || event.key.keysym.sym == SDLK_x) {
          fdd_close(0);
          fdd_close(1);
          fdd_close(2);
          fdd_close(3);
          config_save();
          if(event.key.keysym.sym == SDLK_z) {
            wcscpy(floppyfns[0], L"fdd0.img");
            wcscpy(floppyfns[1], L"fdd1.img");
            wcscpy(floppyfns[2], L"fdd2.img");
            wcscpy(floppyfns[3], L"fdd3.img");
            fdd_load(0, floppyfns[0]);
            fdd_load(1, floppyfns[1]);
            fdd_load(2, floppyfns[2]);
            fdd_load(3, floppyfns[3]);
          }
          continue;
        }
        if(event.key.keysym.sym == SDLK_c) {
          wcscpy(cdrom[0].image_path, L"cdrom.iso");
          cdrom_image_open(&(cdrom[0]), cdrom[0].image_path);
          if (cdrom[0].insert)
            cdrom[0].insert(cdrom[0].priv);
          config_save();
          continue;
        }
        if(event.key.keysym.sym == SDLK_v) {
          cdrom_eject(0);
          continue;
        }
      }
      sdl_keyboard_handler(&event.key);
    }
    if (event.type == SDL_MOUSEMOTION) {
      sdl_mousemotion_handler(&event.motion);
    }
    if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
      if(event.type == SDL_MOUSEBUTTONUP) {
        if(event.button.button == SDL_BUTTON_LEFT) {
          plat_mouse_capture(1);
        }
        if(event.button.button == SDL_BUTTON_MIDDLE
          && mouse_get_buttons() < 3) {
          plat_mouse_capture(0);
        }
      }
      sdl_mousebutton_handler(&event.button);
    }
    if (event.type == SDL_MOUSEWHEEL) {
      sdl_mousewheel_handler(&event.wheel);
    }
  }
}

void mouse_poll(void) {
  static int buttons = 0;
  static int x = 0, y = 0, z = 0;

  if(mouse_capture || video_fullscreen) {
    if(x != sdl_mousedata.dx
      || y != sdl_mousedata.dy || z != sdl_mousedata.dwheel) {
      mouse_x += sdl_mousedata.dx;
      mouse_y += sdl_mousedata.dy;
      mouse_z += sdl_mousedata.dwheel;

      x = sdl_mousedata.dx;
      y = sdl_mousedata.dy;
      z = sdl_mousedata.dwheel;
    }

    if(buttons != sdl_mousedata.buttons) {
      mouse_buttons = sdl_mousedata.buttons;
      buttons = sdl_mousedata.buttons;
    }
  }
}

void plat_mouse_capture(int on) {
  if(on) {
    SDL_SetRelativeMouseMode(1);
    mouse_capture = 1;
  } else {
    SDL_SetRelativeMouseMode(0);
    mouse_capture = 0;
  }
}

wchar_t* ui_window_title(wchar_t* title) {
  char* title_c = malloc_wcstombs(title);
  SDL_SetWindowTitle(sdl_win, title_c);
  free(title_c);
  return NULL;
}
