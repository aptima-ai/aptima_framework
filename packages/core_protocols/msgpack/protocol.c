//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "include_internal/aptima_runtime/addon/protocol/protocol.h"

#include "core_protocols/msgpack/common/constant_str.h"
#include "core_protocols/msgpack/common/parser.h"
#include "core_protocols/msgpack/msg/msg.h"
#include "include_internal/aptima_runtime/addon/addon.h"
#include "include_internal/aptima_runtime/common/constant_str.h"
#include "include_internal/aptima_runtime/metadata/metadata.h"
#include "include_internal/aptima_runtime/protocol/integrated/protocol_integrated.h"
#include "include_internal/aptima_runtime/aptima_env/metadata.h"
#include "aptima_runtime/addon/addon.h"
#include "aptima_runtime/aptima_env/internal/on_xxx_done.h"
#include "aptima_runtime/aptima_env/aptima_env.h"
#include "aptima_utils/lib/alloc.h"
#include "aptima_utils/macro/check.h"
#include "aptima_utils/macro/mark.h"

typedef struct aptima_protocol_msgpack_t {
  aptima_protocol_integrated_t base;

  aptima_msgpack_parser_t parser;
} aptima_protocol_msgpack_t;

static void aptima_protocol_msgpack_on_input(aptima_protocol_msgpack_t *self,
                                          aptima_buf_t input_buf,
                                          aptima_list_t *result_msgs) {
  aptima_ASSERT(self && aptima_protocol_check_integrity(&self->base.base, true),
             "Invalid argument.");
  aptima_ASSERT(input_buf.data && input_buf.content_size && result_msgs,
             "Invalid argument.");

  aptima_msgpack_deserialize_msgs(&self->parser, input_buf, result_msgs);
}

static aptima_buf_t aptima_protocol_msgpack_on_output(aptima_protocol_msgpack_t *self,
                                                aptima_list_t *output_msgs) {
  aptima_ASSERT(self && aptima_protocol_check_integrity(&self->base.base, true),
             "Invalid argument.");
  aptima_ASSERT(self->base.on_output, "Invalid argument.");

  return aptima_msgpack_serialize_msgs(output_msgs, NULL);
}

static void aptima_protocol_msgpack_on_destroy_instance(
    aptima_UNUSED aptima_addon_t *addon, aptima_env_t *aptima_env, void *_self,
    void *context) {
  aptima_protocol_msgpack_t *self = (aptima_protocol_msgpack_t *)_self;
  aptima_ASSERT(self &&
                 // aptima_NOLINTNEXTLINE(thread-check)
                 // thread-check: The belonging thread of the 'protocol' is
                 // ended when this function is called, so we can not check
                 // thread integrity here.
                 aptima_protocol_check_integrity(&self->base.base, false),
             "Invalid argument.");

  aptima_msgpack_parser_deinit(&self->parser);
  aptima_protocol_deinit(&self->base.base);

  aptima_FREE(self);

  aptima_env_on_destroy_instance_done(aptima_env, context, NULL);
}

static void aptima_protocol_msgpack_on_create_instance(
    aptima_UNUSED aptima_addon_t *addon, aptima_env_t *aptima_env,
    aptima_UNUSED const char *name, void *context) {
  aptima_protocol_msgpack_t *self =
      (aptima_protocol_msgpack_t *)aptima_MALLOC(sizeof(aptima_protocol_msgpack_t));
  aptima_ASSERT(self, "Failed to allocate memory.");

  aptima_protocol_integrated_init(
      &self->base, aptima_STR_MSGPACK,
      (aptima_protocol_integrated_on_input_func_t)aptima_protocol_msgpack_on_input,
      (aptima_protocol_integrated_on_output_func_t)aptima_protocol_msgpack_on_output);

  // Configure the retry mechanism.
  self->base.retry_config.enable = true;
  self->base.retry_config.interval_ms = 500;
  self->base.retry_config.max_retries = 5;

  aptima_msgpack_parser_init(&self->parser);

  aptima_env_on_create_instance_done(aptima_env, self, context, NULL);
}

static void aptima_protocol_msgpack_on_init(aptima_UNUSED aptima_addon_t *addon,
                                         aptima_env_t *aptima_env) {
  bool result = aptima_env_init_manifest_from_json(aptima_env,
                                                // clang-format off
                        "{\
                          \"type\": \"protocol\",\
                          \"" aptima_STR_NAME "\": \"" aptima_STR_MSGPACK "\",\
                          \"" aptima_STR_PROTOCOL "\": [\
                            \"" aptima_STR_MSGPACK "\"\
                          ],\
                          \"version\": \"1.0.0\"\
                         }",
                                                // clang-format on
                                                NULL);
  aptima_ASSERT(result, "Should not happen.");

  aptima_env_on_init_done(aptima_env, NULL);
}

static aptima_addon_t msgpack_protocol_factory = {
    NULL,
    aptima_ADDON_SIGNATURE,
    aptima_protocol_msgpack_on_init,
    NULL,
    aptima_protocol_msgpack_on_create_instance,
    aptima_protocol_msgpack_on_destroy_instance,
    NULL,
    NULL,
};

aptima_REGISTER_ADDON_AS_PROTOCOL(msgpack, &msgpack_protocol_factory);
