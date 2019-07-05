#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>
#include <locale.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

#define HAVE_STDARG_H
#include "../86box.h"
#include "../device.h"
#include "../video/video.h"
#define GLOBAL
#include "../plat.h"

#include "linux_sdl.h"

/* Platform public data. */
mutex_t* blit_mutex;

/* Local data. */
static thread_t* main_thread;
static wchar_t* program_name;
static int vid_api_inited = 0;

#define RENDERERS_NUM 1
static struct {
  char* name;
  int   local;
  int   (*init)(void);
  void  (*close)(void);
  void  (*resize)(int x, int y);
  int   (*pause)(void);
} vid_apis[2][RENDERERS_NUM] = {
  {
    { "SDL", 1, sdl_init, sdl_close, NULL, sdl_pause }
  },
  {
    { "SDL", 1, sdl_init_fs, sdl_close, sdl_resize, sdl_pause }
  }
};

char* malloc_wcstombs(const wchar_t* src) {
  int conv_len = (wcslen(src) * 6) + 1;
  char* buf = malloc(sizeof(char) * conv_len);
  wcstombs(buf, src, conv_len);
  return buf;
}

int main(int argc, const char* argv[]) {
  video_fullscreen = 0;
  sprintf(emu_version, "%s v%s", EMU_NAME, EMU_VERSION);

  setlocale(LC_ALL, "POSIX");

  /* Convert command line to wide-chars. */
  wchar_t** argw = alloca(sizeof(wchar_t*) * argc);
  for(int i = 0; i < argc; ++i) {
    int conv_len = strlen(argv[i]) + 1;
    argw[i] = alloca(sizeof(wchar_t) * conv_len);
    mbstowcs(argw[i], argv[i], conv_len);
  }
  program_name = argw[0];

  if(!pc_init(argc, argw)) {
    return 1;
  }

  /* TODO: Basic UI needed, refactor this stuff into UI code */
  blit_mutex = thread_create_mutex(NULL);
  if(!pc_init_modules()) {
    printf("No ROMs found!\n");
    return 6;
  }
  if(!plat_setvid(vid_api)) {
    printf("Failed to start video subsystem.\n");
    return 5;
  }
  plat_resize(scrnsz_x, scrnsz_y);
  pc_reset_hard_init();
  plat_pause(0);
  do_start();
  while(!quited) {
    sdl_eventpump();
  }
  do_stop();

  return 0;
}

void do_start(void) {
  quited = 0;

  /* Setup high-resolution timer. */
  struct timespec ts;
  clock_getres(CLOCK_MONOTONIC, &ts);
  timer_freq = (1000 * 1000 * 1000) / ts.tv_nsec;
  printf("Main timer precision: %lu Hz\n", timer_freq);

  /* Start the emulator, really. */
  main_thread = thread_create(pc_thread, &quited);
}

/* Cleanly stop the emulator. */
void do_stop(void) {
  quited = 1;
  plat_delay_ms(100);
  pc_close(main_thread);
  main_thread = NULL;
}

void plat_get_exe_name(wchar_t* s, int size) {
  wcsncpy(s, program_name, size);
}

void plat_tempfile(wchar_t* bufp, wchar_t* prefix, wchar_t* suffix) {
  char temp[1024];
  if (prefix != NULL) {
    sprintf(temp, "%ls-", prefix);
  } else {
    strcpy(temp, "");
  }

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  struct tm* date = localtime(&ts.tv_sec);
  sprintf(&temp[strlen(temp)], "%d%02d%02d-%02d%02d%02d-%03ld%ls",
    date->tm_year, date->tm_mon, date->tm_mday,
    date->tm_hour, date->tm_min, date->tm_sec,
    ts.tv_nsec / 1000 / 1000, suffix);
  mbstowcs(bufp, temp, strlen(temp) + 1);
}

int plat_getcwd(wchar_t* bufp, int max) {
  char bufp_c[max];
  getcwd(bufp_c, max);
  mbstowcs(bufp, bufp_c, max);

  return 0;
}

int plat_chdir(wchar_t* path) {
  char* path_c = malloc_wcstombs(path);
  int result = chdir(path_c);
  free(path_c);
  return result;
}

FILE* plat_fopen(wchar_t* path, wchar_t* mode) {
  char* path_c = malloc_wcstombs(path);
  char* mode_c = malloc_wcstombs(mode);
  FILE* result = fopen(path_c, mode_c);
  free(path_c);
  free(mode_c);
  return result;
}

void plat_remove(wchar_t* path) {
  char* path_c = malloc_wcstombs(path);
  remove(path_c);
  free(path_c);
}

/* Make sure a path ends with a trailing slash. */
void plat_path_slash(wchar_t *path) {
  if (path[wcslen(path)-1] != L'/') {
      wcscat(path, L"/");
    }
}

/* Check if the given path is absolute or not. */
int plat_path_abs(wchar_t *path) {
    if (path[0] == L'/') return 1;
    return 0;
}

wchar_t* plat_get_filename(wchar_t* s) {
    int c = wcslen(s) - 1;
    while (c > 0) {
      if(s[c] == L'/') return &s[c+1];
      c--;
    }
    return s;
}

void plat_append_filename(wchar_t* dest, wchar_t* s1, wchar_t* s2) {
  wcscat(dest, s1);
  wcscat(dest, s2);
}

/* Actually a forward slash..
 * and this function seems redundant, see plat_path_slash(). */
void plat_put_backslash(wchar_t *s) {
    int c = wcslen(s) - 1;
    if (s[c] != L'/') s[c] = L'/';
}

int plat_dir_check(wchar_t *path) {
  char* path_c = malloc_wcstombs(path);
  struct stat path_stat;
  stat(path_c, &path_stat);
  int result = S_ISDIR(path_stat.st_mode);
  free(path_c);
  return result;
}

int plat_dir_create(wchar_t *path) {
  char* path_c = malloc_wcstombs(path);
  int result = mkdir(path_c, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
  free(path_c);
  return result;
}

wchar_t* plat_get_extension(wchar_t* s) {
  int c = wcslen(s) - 1;
  if (c <= 0) return s;
  while (c && s[c] != L'.') c--;
  if(!c) return &s[wcslen(s)];
  return &s[c+1];
}

/* Any monotonically increasing value will do. */
uint64_t plat_timer_read(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((uint64_t) ts.tv_sec) * 1000 * 1000 * 1000 + ((uint64_t)ts.tv_nsec);
}

/* 1 tick must equal 1 millisecond */
uint32_t plat_get_ticks(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000 / 1000;
}

void plat_delay_ms(uint32_t count) {
  usleep(count * 1000);
}

int plat_vidapi(char* name) {
  int i;

  if (!strcasecmp(name, "default") || !strcasecmp(name, "system")) return 0;

  for(i = 0; i < RENDERERS_NUM; ++i) {
    if (vid_apis[0][i].name && !strcasecmp(vid_apis[0][i].name, name)) return i;
  }

  return 0;
}

char* plat_vidapi_name(int api) {
  char* name = "default";

  switch(api) {
    case 0:
      break;
  }

  return name;
}

int plat_setvid(int api) {
  int i;

  printf("Initializing video API: %s\n", plat_vidapi_name(api));
  startblit();
  video_wait_for_blit();

  /* Close the old API. */
  vid_apis[0][vid_api].close();
  vid_api = api;

  /* TODO: Start the new API. */
  i = vid_apis[0][vid_api].init();
  endblit();
  if (!i) return 0;

  device_force_redraw();
  vid_api_inited = 1;
  return 1;
}

void plat_vidsize(int x, int y) {
  if (!vid_api_inited || !vid_apis[video_fullscreen][vid_api].resize) return;

  startblit();
  video_wait_for_blit();
  vid_apis[video_fullscreen][vid_api].resize(x, y);
  endblit();
}

int get_vidpause(void) {
    return vid_apis[video_fullscreen][vid_api].pause();
}

void startblit(void) {
  thread_wait_mutex(blit_mutex);
}

void endblit(void) {
  thread_release_mutex(blit_mutex);
}
