#include <SDL2/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#undef HAVE_STDARG_H
#define HAVE_STDARG_H
#include "../86box.h"
#include "../plat.h"
#include "../keyboard.h"
#include "../video/video.h"

extern int keyboard_enabled;
#include "linux_sdl.h"

static SDL_Window* sdl_win = NULL;
static SDL_Renderer* sdl_render = NULL;
static SDL_Texture* sdl_tex = NULL;
static SDL_Event event;
static int sdl_fs;
static int cur_w, cur_h;

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

  SDL_LockTexture(sdl_tex, 0, &pixeldata, &pitch);

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
    printf("SDL: unable to copy texture to renderer (%s)\n", SDL_GetError());
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

  printf("SDL: init (fs=%d)\n", fs);

  cgapal_rebuild();

  SDL_GetVersion(&ver);
  printf("SDL: version %d.%d.%d\n", ver.major, ver.minor, ver.patch);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL: initialization failed (%s)\n", SDL_GetError());
    return 0;
  }

  sprintf(temp, "%s v%s", EMU_NAME, EMU_VERSION);
  sdl_win = SDL_CreateWindow(temp,
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    scrnsz_x, scrnsz_y, fs ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  mouse_capture = 0;
  if(sdl_win == NULL) {
    printf("SDL: unable to create window (%s)\n", SDL_GetError());
    sdl_close();
    return 0;
  }

  sdl_render = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_SOFTWARE);
  if(sdl_render == NULL) {
    printf("SDL: unable to create renderer (%s)\n", SDL_GetError());
    sdl_close();
    return 0;
  }

  sdl_tex = SDL_CreateTexture(sdl_render, SDL_PIXELFORMAT_ABGR8888,
    SDL_TEXTUREACCESS_STREAMING, 2048, 2048);
  if(sdl_tex == NULL) {
    printf("SDL: unable to create texture (%s)\n", SDL_GetError());
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

  printf("SDL: resize (%i, %i)\n", x, y);
  SDL_SetWindowSize(sdl_win, x, y);
  /* TODO: fs mode scaling? */

  cur_w = x;
  cur_h = y;
}
void plat_resize(int x, int y) { sdl_resize(x, y); }

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

void sdl_eventpump(void) {
  while(SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      quited = 1;
    }
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      sdl_keyboard_handler(&event.key);
    }
  }
}
