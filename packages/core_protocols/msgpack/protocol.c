//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "include_internal/axis_runtime/addon/protocol/protocol.h"

#include "core_protocols/msgpack/common/constant_str.h"
#include "core_protocols/msgpack/common/parser.h"
#include "core_protocols/msgpack/msg/msg.h"
#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/metadata/metadata.h"
#include "include_internal/axis_runtime/protocol/integrated/protocol_integrated.h"
#include "include_internal/axis_runtime/axis_env/metadata.h"
#include "axis_runtime/addon/addon.h"
#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

typedef struct axis_protocol_msgpack_t {
  axis_protocol_integrated_t base;

  axis_msgpack_parser_t parser;
} axis_protocol_msgpack_t;

static void axis_protocol_msgpack_on_input(axis_protocol_msgpack_t *self,
                                          axis_buf_t input_buf,
                                          axis_list_t *result_msgs) {
  axis_ASSERT(self && axis_protocol_check_integrity(&self->base.base, true),
             "Invalid argument.");
  axis_ASSERT(input_buf.data && input_buf.content_size && result_msgs,
             "Invalid argument.");

  axis_msgpack_deserialize_msgs(&self->parser, input_buf, result_msgs);
}

static axis_buf_t axis_protocol_msgpack_on_output(axis_protocol_msgpack_t *self,
                                                axis_list_t *output_msgs) {
  axis_ASSERT(self && axis_protocol_check_integrity(&self->base.base, true),
             "Invalid argument.");
  axis_ASSERT(self->base.on_output, "Invalid argument.");

  return axis_msgpack_serialize_msgs(output_msgs, NULL);
}

static void axis_protocol_msgpack_on_destroy_instance(
    axis_UNUSED axis_addon_t *addon, axis_env_t *axis_env, void *_self,
    void *context) {
  axis_protocol_msgpack_t *self = (axis_protocol_msgpack_t *)_self;
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: The belonging thread of the 'protocol' is
                 // ended when this function is called, so we can not check
                 // thread integrity here.
                 axis_protocol_check_integrity(&self->base.base, false),
             "Invalid argument.");

  axis_msgpack_parser_deinit(&self->parser);
  axis_protocol_deinit(&self->base.base);

  axis_FREE(self);

  axis_env_on_destroy_instance_done(axis_env, context, NULL);
}

static void axis_protocol_msgpack_on_create_instance(
    axis_UNUSED axis_addon_t *addon, axis_env_t *axis_env,
    axis_UNUSED const char *name, void *context) {
  axis_protocol_msgpack_t *self =
      (axis_protocol_msgpack_t *)axis_MALLOC(sizeof(axis_protocol_msgpack_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_protocol_integrated_init(
      &self->base, axis_STR_MSGPACK,
      (axis_protocol_integrated_on_input_func_t)axis_protocol_msgpack_on_input,
      (axis_protocol_integrated_on_output_func_t)axis_protocol_msgpack_on_output);

  // Configure the retry mechanism.
  self->base.retry_config.enable = true;
  self->base.retry_config.interval_ms = 500;
  self->base.retry_config.max_retries = 5;

  axis_msgpack_parser_init(&self->parser);

  axis_env_on_create_instance_done(axis_env, self, context, NULL);
}

static void axis_protocol_msgpack_on_init(axis_UNUSED axis_addon_t *addon,
                                         axis_env_t *axis_env) {
  bool result = axis_env_init_manifest_from_json(axis_env,
                                                // clang-format off
                        "{\
                          \"type\": \"protocol\",\
                          \"" axis_STR_NAME "\": \"" axis_STR_MSGPACK "\",\
                          \"" axis_STR_PROTOCOL "\": [\
                            \"" axis_STR_MSGPACK "\"\
                          ],\
                          \"version\": \"1.0.0\"\
                         }",
                                                // clang-format on
                                                NULL);
  axis_ASSERT(result, "Should not happen.");

  axis_env_on_init_done(axis_env, NULL);
}

static axis_addon_t msgpack_protocol_factory = {
    NULL,
    axis_ADDON_SIGNATURE,
    axis_protocol_msgpack_on_init,
    NULL,
    axis_protocol_msgpack_on_create_instance,
    axis_protocol_msgpack_on_destroy_instance,
    NULL,
    NULL,
};

axis_REGISTER_ADDON_AS_PROTOCOL(msgpack, &msgpack_protocol_factory);
