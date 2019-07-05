#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#define HAVE_STDARG_H
#include "../86box.h"
#include "../plat_dynld.h"

void* dynld_module(const char* name, dllimp_t* table) {
  void* h;
  void* func;
  dllimp_t* imp;

  if ((h = dlopen(name, RTLD_LAZY)) == NULL) {
    printf("DynLd(\"%s\"): library not found!\n", name);
    return NULL;
  }

  for(imp = table; imp->name != NULL; ++imp) {
    func = dlsym(h, imp->name);
    if(func == NULL) {
      printf("DynLd(\"%s\"): function '%s' not found!\n", name, imp->name);
      dlclose(h);
      return NULL;
    }
    *((char**)imp->func) = (char*)func;
  }

  return h;
}

void dynld_close(void* handle) {
  if (handle != NULL) dlclose(handle);
}
