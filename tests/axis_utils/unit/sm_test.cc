//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "aptima_utils/lib/sm.h"

#include <memory>

#include "gtest/gtest.h"

struct sm_ctx {
  int do_nothing_times;
  int do_action1_times;
  int do_action2_times;
};

TEST(StateMachine, positive) {
  auto ctx = std::make_unique<sm_ctx>();
  ctx->do_action1_times = ctx->do_action2_times = ctx->do_nothing_times = 0;

  auto sm = aptima_state_machine_create();
  EXPECT_NE(sm, nullptr);

  auto action1 = [](aptima_sm_t *sm, const aptima_sm_state_history_t *top,
                    void *arg) {
    (void)sm;
    (void)top;
    auto *c = static_cast<sm_ctx *>(arg);
    c->do_action1_times++;
  };

  auto action2 = [](aptima_sm_t *sm, const aptima_sm_state_history_t *top,
                    void *arg) {
    (void)sm;
    (void)top;
    auto *c = static_cast<sm_ctx *>(arg);
    c->do_action2_times++;
  };

  auto nothing = [](aptima_sm_t *sm, const aptima_sm_state_history_t *top,
                    void *arg) {
    (void)sm;
    (void)top;
    auto *c = static_cast<sm_ctx *>(arg);
    c->do_nothing_times++;
  };

  aptima_sm_state_entry_t entries[] = {{0, 0, -1, 1, action1},
                                    {1, 1, -1, 2, action2}};

  auto ret = aptima_state_machine_init(
      sm, 0, nothing, entries, sizeof(entries) / sizeof(entries[0]), NULL, 0);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(aptima_state_machine_trigger(sm, 0, 0, ctx.get()), 0);
  EXPECT_EQ(ctx->do_action1_times, 1);
  EXPECT_EQ(aptima_state_machine_trigger(sm, 1, 0, ctx.get()), 0);
  EXPECT_EQ(ctx->do_action2_times, 1);
  EXPECT_EQ(aptima_state_machine_trigger(sm, 2, 0, ctx.get()), 0);
  EXPECT_EQ(ctx->do_nothing_times, 1);
  aptima_state_machine_destroy(sm);
}