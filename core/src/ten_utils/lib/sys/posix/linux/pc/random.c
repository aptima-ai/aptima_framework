//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/random.h"

#include <assert.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "ten_utils/lib/thread_once.h"

static int g_random_fd = -1;
static ten_thread_once_t g_init = TEN_THREAD_ONCE_INIT;

static void ten_init_random(void) {
  g_random_fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
}

int ten_random(void *buf, size_t size) {
  if (!buf || !size) {
    return -1;
  }

  ten_thread_once(&g_init, ten_init_random);

  if (g_random_fd < 0) {
    return -1;
  }

  ssize_t ret = read(g_random_fd, buf, size);

  return ret == size ? 0 : -1;
}
