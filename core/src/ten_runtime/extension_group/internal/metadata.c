//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/extension_group/metadata.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_group/extension_group_info/extension_group_info.h"
#include "include_internal/ten_runtime/extension_group/on_xxx.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "ten_utils/macro/check.h"

void ten_extension_group_load_metadata(ten_extension_group_t *self) {
  TEN_ASSERT(self &&
                 // This function is safe to be called from the extension
                 // threads, because all the resources it accesses are not be
                 // modified after the app initialization phase.
                 ten_extension_group_check_integrity(self, true),
             "Should not happen.");

  TEN_LOGD("[%s] Load metadata.", ten_extension_group_get_name(self));

  if (self->addon_host) {
    // If the extension group is created by an addon, then the base directory of
    // the extension group can be set to
    // `<app>/ten_packages/extension_group/<addon-name>`. And the `base_dir`
    // must be set before `on_init()`, as the `property.json` and
    // `manifest.json` under the `base_dir` might be loaded in the default
    // behavior of `on_init()`, or users might want to know the value of
    // `base_dir` in `on_init()`.
    ten_addon_host_set_base_dir(self->addon_host, self->app, &self->base_dir);
  }

  ten_metadata_load(ten_extension_group_on_init, self->ten_env);
}

void ten_extension_group_merge_properties_from_graph(
    ten_extension_group_t *self) {
  TEN_ASSERT(self && ten_extension_group_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(self->extension_group_info, "Invalid argument.");

  // Merge properties in extension_group_info_from_graph into the extension
  // group's property store.
  if (self->extension_group_info->property) {
    ten_value_object_merge_with_clone(&self->property,
                                      self->extension_group_info->property);
  }
}
