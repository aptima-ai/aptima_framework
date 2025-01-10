//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <thread>

#include "gtest/gtest.h"
#include "aptima_utils/lib/event.h"
#include "aptima_utils/lib/thread.h"
#include "aptima_utils/lib/thread_local.h"

static void *dummy_routine(void *) { return nullptr; }

TEST(ThreadTest, negative) {
  // no routine, not a valid thread
  auto t = aptima_thread_create(nullptr, nullptr, nullptr);
  EXPECT_EQ(t, nullptr);

  t = aptima_thread_create(nullptr, dummy_routine, nullptr);
  EXPECT_NE(t, nullptr);

  EXPECT_EQ(aptima_thread_join(t, -1), 0);

  EXPECT_NE(aptima_thread_join(nullptr, 0), 0);
}

struct simple_routine_data {
  aptima_thread_t *thread;
  aptima_event_t *go;
  aptima_tid_t tid;
};

static void *simple_routine(void *args) {
  simple_routine_data *t = (simple_routine_data *)args;
  aptima_event_wait(t->go, -1);

  EXPECT_NE(args, nullptr);
  EXPECT_NE(aptima_thread_self(), nullptr);
  EXPECT_EQ(aptima_thread_self(), t->thread);
  EXPECT_NE(aptima_thread_get_id(nullptr), 0);
  EXPECT_EQ(aptima_thread_get_id(nullptr), t->tid);
  EXPECT_EQ(aptima_thread_get_id(aptima_thread_self()), t->tid);
  return nullptr;
}

TEST(ThreadTest, positive) {
  EXPECT_EQ(aptima_thread_self(), nullptr);

  auto create_thread_task = [] {
    simple_routine_data data = {0};
    data.go = aptima_event_create(0, 0);
    aptima_thread_t *t = aptima_thread_create(nullptr, simple_routine, &data);
    EXPECT_NE(t, nullptr);
    EXPECT_NE(aptima_thread_get_id(t), 0);
    data.thread = t;
    data.tid = aptima_thread_get_id(t);
    aptima_event_set(data.go);
    aptima_thread_join(t, -1);
    aptima_event_destroy(data.go);
  };

  std::thread threads[10];
  for (auto i = 0; i < 10; i++) {
    threads[i] = std::thread(create_thread_task);
  }

  for (auto i = 0; i < 10; i++) {
    threads[i].join();
  }
}

TEST(ThreadLocalTest, negative) {
  EXPECT_EQ(aptima_thread_self(), nullptr);
  EXPECT_EQ(aptima_thread_get_key(kInvalidTlsKey), nullptr);
  EXPECT_EQ(aptima_thread_set_key(kInvalidTlsKey, nullptr), -1);
}

TEST(ThreadLocalTest, positive) {
  auto key = aptima_thread_key_create();
  EXPECT_NE(key, kInvalidTlsKey);
  EXPECT_EQ(aptima_thread_get_key(key), nullptr);
  EXPECT_EQ(aptima_thread_set_key(key, (void *)0xdeadbeef), 0);
  EXPECT_EQ(aptima_thread_get_key(key), (void *)0xdeadbeef);
  aptima_thread_key_destroy(key);
  EXPECT_EQ(aptima_thread_get_key(key), nullptr);

  aptima_event_t *t1_ready, *t2_ready;
  aptima_event_t *go;
  aptima_event_t *t1_done, *t2_done;
  key = kInvalidTlsKey;

  t1_ready = aptima_event_create(0, 0);
  t2_ready = aptima_event_create(0, 0);
  go = aptima_event_create(0, 0);
  t1_done = aptima_event_create(0, 0);
  t2_done = aptima_event_create(0, 0);

  auto task1 = [t1_ready, go, t1_done, t2_done, &key] {
    aptima_event_set(t1_ready);
    aptima_event_wait(go, -1);
    key = aptima_thread_key_create();
    EXPECT_EQ(aptima_thread_set_key(key, (void *)0xdeadbee1), 0);
    aptima_event_set(t1_done);
    aptima_event_wait(t2_done, -1);
    EXPECT_EQ(aptima_thread_get_key(key), (void *)0xdeadbee1);
  };

  auto task2 = [t2_ready, go, t1_done, t2_done, &key] {
    aptima_event_set(t2_ready);
    aptima_event_wait(go, -1);
    aptima_event_wait(t1_done, -1);
    EXPECT_EQ(aptima_thread_get_key(key), nullptr);
    EXPECT_EQ(aptima_thread_set_key(key, (void *)0xdeadbee2), 0);
    aptima_event_set(t2_done);
    EXPECT_EQ(aptima_thread_get_key(key), (void *)0xdeadbee2);
  };

  auto t1 = std::thread(task1);
  auto t2 = std::thread(task2);

  aptima_event_wait(t1_ready, -1);
  aptima_event_wait(t2_ready, -1);

  aptima_event_set(go);

  t1.join();
  t2.join();

  aptima_event_destroy(t1_ready);
  aptima_event_destroy(t2_ready);
  aptima_event_destroy(go);
  aptima_event_destroy(t1_done);
  aptima_event_destroy(t2_done);
}
