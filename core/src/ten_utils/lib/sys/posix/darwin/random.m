//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/random.h"

#import <Foundation/Foundation.h>
#import <Security/Security.h>

int ten_random(void *buf, size_t size) {
  return SecRandomCopyBytes(kSecRandomDefault, size, buf);
}
