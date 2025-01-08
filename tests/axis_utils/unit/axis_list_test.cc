//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <cstdint>
#include <iostream>

#include "common/test_utils.h"
#include "gtest/gtest.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"

namespace {

int compare_int32(axis_listnode_t *x, axis_listnode_t *y) {
  axis_int32_listnode_t *ix = axis_listnode_to_int32_listnode(x);
  axis_int32_listnode_t *iy = axis_listnode_to_int32_listnode(y);

  return iy->int32 - ix->int32;
}

}  // namespace

TEST(TenListTest, OrderedInsert) {  // NOLINT
  // Insert into an empty list
  axis_list_t list = axis_LIST_INIT_VAL;

  axis_listnode_t *one = axis_int32_listnode_create(1);
  axis_list_push_back_in_order(&list, one, compare_int32, false);

  EXPECT_EQ(axis_list_size(&list), 1);

  axis_int32_listnode_t *front =
      axis_listnode_to_int32_listnode(axis_list_front(&list));
  EXPECT_EQ(front->int32, 1);

  axis_int32_listnode_t *back =
      axis_listnode_to_int32_listnode(axis_list_back(&list));
  EXPECT_EQ(back->int32, 1);

  // Insert front
  axis_listnode_t *zero = axis_int32_listnode_create(0);
  axis_list_push_back_in_order(&list, zero, compare_int32, false);

  EXPECT_EQ(axis_list_size(&list), 2);

  front = axis_listnode_to_int32_listnode(axis_list_front(&list));
  EXPECT_EQ(front->int32, 0);

  back = axis_listnode_to_int32_listnode(axis_list_back(&list));
  EXPECT_EQ(back->int32, 1);

  // Insert back
  axis_listnode_t *three = axis_int32_listnode_create(3);
  axis_list_push_back_in_order(&list, three, compare_int32, false);

  EXPECT_EQ(axis_list_size(&list), 3);

  front = axis_listnode_to_int32_listnode(axis_list_front(&list));
  EXPECT_EQ(front->int32, 0);

  back = axis_listnode_to_int32_listnode(axis_list_back(&list));
  EXPECT_EQ(back->int32, 3);

  // Insert middle
  axis_listnode_t *two = axis_int32_listnode_create(2);
  axis_list_push_back_in_order(&list, two, compare_int32, false);

  EXPECT_EQ(axis_list_size(&list), 4);

  axis_int32_listnode_t *node_two =
      axis_listnode_to_int32_listnode(axis_list_back(&list)->prev);
  EXPECT_EQ(node_two->int32, 2);

  // Insert existed value
  axis_listnode_t *another_one = axis_int32_listnode_create(1);
  axis_listnode_t *another_three = axis_int32_listnode_create(3);
  axis_list_push_back_in_order(&list, another_one, compare_int32, false);
  axis_list_push_back_in_order(&list, another_three, compare_int32, false);

  EXPECT_EQ(axis_list_size(&list), 6);
  EXPECT_EQ(axis_listnode_to_int32_listnode(axis_list_front(&list)->next)->int32,
            1);
  EXPECT_EQ(axis_listnode_to_int32_listnode(axis_list_back(&list)->prev)->int32,
            3);

  // Debug log. [0,1,1,2,3,3]
  // axis_list_foreach (&list, iter) {
  //   std::cout << axis_listnode_to_int32_listnode(iter.node)->int32 << " ";
  // }
  // std::cout << std::endl;

  axis_list_clear(&list);
}

TEST(TenListTest, IteratorNext) {  // NOLINT
  axis_list_t list = axis_LIST_INIT_VAL;

  axis_listnode_t *one = axis_int32_listnode_create(1);
  axis_list_push_back(&list, one);

  axis_listnode_t *two = axis_int32_listnode_create(2);
  axis_list_push_back(&list, two);

  axis_listnode_t *three = axis_int32_listnode_create(3);
  axis_list_push_back(&list, three);

  axis_list_iterator_t iter = axis_list_begin(&list);
  EXPECT_EQ(1, axis_listnode_to_int32_listnode(iter.node)->int32);

  iter = axis_list_iterator_next(iter);
  EXPECT_EQ(2, axis_listnode_to_int32_listnode(iter.node)->int32);

  iter = axis_list_iterator_next(iter);
  EXPECT_EQ(3, axis_listnode_to_int32_listnode(iter.node)->int32);

  iter = axis_list_iterator_next(iter);
  EXPECT_EQ(NULL, iter.node);

  axis_list_clear(&list);
}
