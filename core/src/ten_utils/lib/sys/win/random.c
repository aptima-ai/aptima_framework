//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/random.h"

#include <Windows.h>
#include <bcrypt.h>

int ten_random(void *buf, size_t size) {
  return (int)BCryptGenRandom(NULL, buf, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
}
