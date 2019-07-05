#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "../plat.h"

typedef struct {
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  bool state;
} pthread_event_t;

thread_t* thread_create(void (*func)(void*), void* param) {
  pthread_t* thread = (pthread_t*) malloc(sizeof(pthread_t));

  pthread_create(thread, NULL, (void* (*)(void*))func, param);

  return ((thread_t*)thread);
}

void thread_kill(void* arg) {
  if (arg == NULL) return;

  pthread_cancel(*((pthread_t*)arg));
  free(arg);
}

int thread_wait(thread_t* arg, int timeout) {
  if (arg == NULL) return 0;

  /* Ignore the timeout, always do a blocking wait on POSIX. */
  pthread_join(*((pthread_t*)arg), NULL);

  return 0;
}

event_t* thread_create_event(void) {
  pthread_event_t* ev = (pthread_event_t*) malloc(sizeof(pthread_event_t));

  pthread_cond_init(&ev->cond, NULL);
  pthread_mutex_init(&ev->mutex, NULL);
  ev->state = false;

  return ((event_t*)ev);
}

void thread_set_event(event_t* arg) {
  pthread_event_t* ev = (pthread_event_t*) arg;
  if(arg == NULL) return;

  pthread_mutex_lock(&ev->mutex);
  ev->state = true;
  if (ev->state) {
    pthread_mutex_unlock(&ev->mutex);
    pthread_cond_signal(&ev->cond);
  }
}

void thread_reset_event(event_t* arg) {
  pthread_event_t* ev = (pthread_event_t*) arg;
  if (arg == NULL) return;

  pthread_mutex_lock(&ev->mutex);
  ev->state = false;
  pthread_mutex_unlock(&ev->mutex);
}

int thread_wait_event(event_t* arg, int timeout) {
  pthread_event_t* ev = (pthread_event_t*) arg;
  if (arg == NULL) return 0;

  if (timeout == 0) {
    if(pthread_mutex_trylock(&ev->mutex) == EBUSY) return 1;
  } else {
    pthread_mutex_lock(&ev->mutex);
  }

  int result = 0;
  if(!ev->state) {
    if (timeout == 0) {
      pthread_mutex_unlock(&ev->mutex);
      return 1;
    }
    struct timespec ts;
    if (timeout != -1) {
      uint64_t nanoseconds = plat_timer_read() + (timeout * 1000 * 1000);
      ts.tv_sec = nanoseconds / 1000 / 1000 / 1000;
      ts.tv_nsec = nanoseconds - ((uint64_t)ts.tv_sec) * 1000 * 1000 * 1000;
    }

    do {
      if (timeout != -1) {
        result = pthread_cond_timedwait(&ev->cond, &ev->mutex, &ts);
      } else {
        result = pthread_cond_wait(&ev->cond, &ev->mutex);
      }
    } while (result == 0 && !ev->state);

    if (result == 0) ev->state = false;
  } else {
    result = 0;
    ev->state = false;
  }

  pthread_mutex_unlock(&ev->mutex);
  return result;
}

void thread_destroy_event(event_t* arg) {
  pthread_event_t* ev = (pthread_event_t*) arg;
  if (arg == NULL) return;

  pthread_cond_destroy(&ev->cond);
  pthread_mutex_destroy(&ev->mutex);
  free(ev);
}

/* TODO: mutex name exclusion?? */

mutex_t* thread_create_mutex(wchar_t* name) {
  pthread_mutex_t* mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));

  pthread_mutex_init(mutex, NULL);

  return ((mutex_t*)mutex);
}

void thread_close_mutex(mutex_t* mutex) {
  if (mutex == NULL) return;

  pthread_mutex_destroy(mutex);
  free(mutex);
}

int thread_wait_mutex(mutex_t* mutex) {
  if (mutex == NULL) return 0;

  pthread_mutex_lock(mutex);

  return 0;
}

int thread_release_mutex(mutex_t* mutex) {
  if (mutex == NULL) return 0;

  pthread_mutex_unlock(mutex);

  return 0;
}
