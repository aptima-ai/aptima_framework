//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <cstdint>
#include <iostream>

#include "common/test_utils.h"
#include "gtest/gtest.h"
#include "aptima_utils/container/list.h"
#include "aptima_utils/container/list_node.h"

namespace {

int compare_int32(aptima_listnode_t *x, aptima_listnode_t *y) {
  aptima_int32_listnode_t *ix = aptima_listnode_to_int32_listnode(x);
  aptima_int32_listnode_t *iy = aptima_listnode_to_int32_listnode(y);

  return iy->int32 - ix->int32;
}

}  // namespace

TEST(TenListTest, OrderedInsert) {  // NOLINT
  // Insert into an empty list
  aptima_list_t list = aptima_LIST_INIT_VAL;

  aptima_listnode_t *one = aptima_int32_listnode_create(1);
  aptima_list_push_back_in_order(&list, one, compare_int32, false);

  EXPECT_EQ(aptima_list_size(&list), 1);

  aptima_int32_listnode_t *front =
      aptima_listnode_to_int32_listnode(aptima_list_front(&list));
  EXPECT_EQ(front->int32, 1);

  aptima_int32_listnode_t *back =
      aptima_listnode_to_int32_listnode(aptima_list_back(&list));
  EXPECT_EQ(back->int32, 1);

  // Insert front
  aptima_listnode_t *zero = aptima_int32_listnode_create(0);
  aptima_list_push_back_in_order(&list, zero, compare_int32, false);

  EXPECT_EQ(aptima_list_size(&list), 2);

  front = aptima_listnode_to_int32_listnode(aptima_list_front(&list));
  EXPECT_EQ(front->int32, 0);

  back = aptima_listnode_to_int32_listnode(aptima_list_back(&list));
  EXPECT_EQ(back->int32, 1);

  // Insert back
  aptima_listnode_t *three = aptima_int32_listnode_create(3);
  aptima_list_push_back_in_order(&list, three, compare_int32, false);

  EXPECT_EQ(aptima_list_size(&list), 3);

  front = aptima_listnode_to_int32_listnode(aptima_list_front(&list));
  EXPECT_EQ(front->int32, 0);

  back = aptima_listnode_to_int32_listnode(aptima_list_back(&list));
  EXPECT_EQ(back->int32, 3);

  // Insert middle
  aptima_listnode_t *two = aptima_int32_listnode_create(2);
  aptima_list_push_back_in_order(&list, two, compare_int32, false);

  EXPECT_EQ(aptima_list_size(&list), 4);

  aptima_int32_listnode_t *node_two =
      aptima_listnode_to_int32_listnode(aptima_list_back(&list)->prev);
  EXPECT_EQ(node_two->int32, 2);

  // Insert existed value
  aptima_listnode_t *another_one = aptima_int32_listnode_create(1);
  aptima_listnode_t *another_three = aptima_int32_listnode_create(3);
  aptima_list_push_back_in_order(&list, another_one, compare_int32, false);
  aptima_list_push_back_in_order(&list, another_three, compare_int32, false);

  EXPECT_EQ(aptima_list_size(&list), 6);
  EXPECT_EQ(aptima_listnode_to_int32_listnode(aptima_list_front(&list)->next)->int32,
            1);
  EXPECT_EQ(aptima_listnode_to_int32_listnode(aptima_list_back(&list)->prev)->int32,
            3);

  // Debug log. [0,1,1,2,3,3]
  // aptima_list_foreach (&list, iter) {
  //   std::cout << aptima_listnode_to_int32_listnode(iter.node)->int32 << " ";
  // }
  // std::cout << std::endl;

  aptima_list_clear(&list);
}

TEST(TenListTest, IteratorNext) {  // NOLINT
  aptima_list_t list = aptima_LIST_INIT_VAL;

  aptima_listnode_t *one = aptima_int32_listnode_create(1);
  aptima_list_push_back(&list, one);

  aptima_listnode_t *two = aptima_int32_listnode_create(2);
  aptima_list_push_back(&list, two);

  aptima_listnode_t *three = aptima_int32_listnode_create(3);
  aptima_list_push_back(&list, three);

  aptima_list_iterator_t iter = aptima_list_begin(&list);
  EXPECT_EQ(1, aptima_listnode_to_int32_listnode(iter.node)->int32);

  iter = aptima_list_iterator_next(iter);
  EXPECT_EQ(2, aptima_listnode_to_int32_listnode(iter.node)->int32);

  iter = aptima_list_iterator_next(iter);
  EXPECT_EQ(3, aptima_listnode_to_int32_listnode(iter.node)->int32);

  iter = aptima_list_iterator_next(iter);
  EXPECT_EQ(NULL, iter.node);

  aptima_list_clear(&list);
}
