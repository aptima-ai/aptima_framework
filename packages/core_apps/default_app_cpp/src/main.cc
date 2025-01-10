//
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "axis_runtime/binding/cpp/aptima.h"

namespace aptima {

class default_app_t : public aptima::app_t {};

}  // namespace aptima

int main() {
  auto *app = new aptima::default_app_t();
  app->run();
  delete app;
  return 0;
}
