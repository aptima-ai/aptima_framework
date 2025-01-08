//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stddef.h>

#include "axis_utils/container/list_int32.h"  // IWYU pragma: keep
#include "axis_utils/container/list_node.h"
#include "axis_utils/container/list_ptr.h"        // IWYU pragma: keep
#include "axis_utils/container/list_smart_ptr.h"  // IWYU pragma: keep
#include "axis_utils/container/list_str.h"        // IWYU pragma: keep
#include "axis_utils/lib/signature.h"

#define axis_LIST_SIGNATURE 0x5D6CC60B9833B104U
#define axis_LIST_LOOP_MAX_ALLOWABLE_CNT 100000

#define axis_list_foreach(list, iter)                                           \
  axis_ASSERT(axis_list_size(list) <= axis_LIST_LOOP_MAX_ALLOWABLE_CNT,           \
             "The time complexity is too high.");                              \
  for (axis_list_iterator_t iter = {NULL, axis_list_front(list),                 \
                                   axis_list_front(list)                        \
                                       ? axis_list_front(list)->next            \
                                       : NULL,                                 \
                                   0};                                         \
       (iter).node;                                                            \
       ++((iter).index), (iter).prev = (iter).node, (iter).node = (iter).next, \
                           (iter).next = (iter).node ? ((iter).node)->next     \
                                                     : NULL)

#define axis_LIST_INIT_VAL                                                   \
  (axis_list_t) {                                                            \
    .signature = axis_LIST_SIGNATURE, .size = 0, .front = NULL, .back = NULL \
  }

typedef struct axis_list_t {
  axis_signature_t signature;
  size_t size;
  axis_listnode_t *front;
  axis_listnode_t *back;
} axis_list_t;

typedef struct axis_list_iterator_t {
  axis_listnode_t *prev;
  axis_listnode_t *node;
  axis_listnode_t *next;
  size_t index;
} axis_list_iterator_t;

axis_UTILS_API bool axis_list_check_integrity(axis_list_t *self);

/**
 * @brief Create a list object.
 * @return A pointer to the list object.
 */
axis_UTILS_API axis_list_t *axis_list_create(void);

/**
 * @brief Destroy a list object and release the memory.
 * @param self The list object.
 */
axis_UTILS_API void axis_list_destroy(axis_list_t *self);

/**
 * @brief Initialize a list.
 * @param self The list to be initialized.
 */
axis_UTILS_API void axis_list_init(axis_list_t *self);

/**
 * @brief Reset a list to an empty list.
 * @param self The list to be reset.
 */
axis_UTILS_API void axis_list_reset(axis_list_t *self);

/**
 * @brief Clear a list and release all the nodes.
 * @param self The list to be cleared.
 */
axis_UTILS_API void axis_list_clear(axis_list_t *self);

/**
 * @brief Check if a list is empty.
 * @param self The list to be checked.
 * @return true if the list is empty, false otherwise.
 */
axis_UTILS_API bool axis_list_is_empty(axis_list_t *self);

/**
 * @brief Get the size of a list.
 * @param self The list to be checked.
 * @return the size of the list.
 */
axis_UTILS_API size_t axis_list_size(axis_list_t *self);

/**
 * @brief Swap two lists.
 * @param self The list to be swapped.
 * @param target The target list to be swapped.
 */
axis_UTILS_API void axis_list_swap(axis_list_t *self, axis_list_t *target);

/**
 * @brief Concatenate two lists.
 * @param self The list to be concatenated.
 * @param target The target list to be concatenated.
 */
axis_UTILS_API void axis_list_concat(axis_list_t *self, axis_list_t *target);

/**
 * @brief Remove a node from a list and keep node memory.
 * @param self The list to be removed from.
 * @param node The node to be removed.
 */
axis_UTILS_API void axis_list_detach_node(axis_list_t *self, axis_listnode_t *node);

/**
 * @brief Remove a node from a list and release node memory.
 * @param self The list to be removed from.
 * @param node The node to be removed.
 */
axis_UTILS_API void axis_list_remove_node(axis_list_t *self, axis_listnode_t *node);

/**
 * @brief Get the front node of a list.
 * @param self The list to be checked.
 * @return the front node of the list. NULL if the list is empty.
 */
axis_UTILS_API axis_listnode_t *axis_list_front(axis_list_t *self);

/**
 * @brief Get the back node of a list.
 * @param self The list to be checked.
 * @return the back node of the list. NULL if the list is empty.
 */
axis_UTILS_API axis_listnode_t *axis_list_back(axis_list_t *self);

/**
 * @brief Push a node to the front of a list.
 * @param self The list to be pushed to.
 * @param node The node to be pushed.
 */
axis_UTILS_API void axis_list_push_front(axis_list_t *self, axis_listnode_t *node);

/**
 * @brief Push a node to the back of a list.
 * @param self The list to be pushed to.
 * @param node The node to be pushed.
 */
axis_UTILS_API void axis_list_push_back(axis_list_t *self, axis_listnode_t *node);

/**
 * @brief Pop the front node of a list.
 * @param self The list to be popped from.
 * @return the front node of the list. NULL if the list is empty.
 */
axis_UTILS_API axis_listnode_t *axis_list_pop_front(axis_list_t *self);

axis_UTILS_API void axis_list_remove_front(axis_list_t *self);

/**
 * @brief Pop the back node of a list.
 * @param self The list to be popped from.
 * @return the back node of the list. NULL if the list is empty.
 */
axis_UTILS_API axis_listnode_t *axis_list_pop_back(axis_list_t *self);

/**
 * @return
 *  * 1 if left > right
 *  * 0 if left == right
 *  * -1 if left < right
 */
typedef int (*axis_list_node_compare_func_t)(axis_listnode_t *left,
                                            axis_listnode_t *right);

/**
 * @brief Insert a node into a list in order. If the compare function cmp(x, y)
 * returns true, then the node x will stand before the node y in the list.
 *
 * @param self The list to be inserted to.
 * @param node The node to be inserted.
 * @param cmp The compare function.
 * @param skip_if_same If skip_if_same is true, this node won't be pushed into
 * the list if one item in the list equals to it (i.e., cmp(x, node) == 0), and
 * this function will return false.
 *
 * @return Whether this node has been pushed into the list. You have to care
 * about the memory of this node if return false.
 */
axis_UTILS_API bool axis_list_push_back_in_order(axis_list_t *self,
                                               axis_listnode_t *node,
                                               axis_list_node_compare_func_t cmp,
                                               bool skip_if_same);

axis_UTILS_API axis_list_iterator_t axis_list_begin(axis_list_t *self);

axis_UTILS_API axis_list_iterator_t axis_list_end(axis_list_t *self);

axis_UTILS_API axis_list_iterator_t
axis_list_iterator_next(axis_list_iterator_t self);

axis_UTILS_API axis_list_iterator_t
axis_list_iterator_prev(axis_list_iterator_t self);

axis_UTILS_API bool axis_list_iterator_is_end(axis_list_iterator_t self);

axis_UTILS_API axis_listnode_t *axis_list_iterator_to_listnode(
    axis_list_iterator_t self);

axis_UTILS_API void axis_list_reverse_new(axis_list_t *src, axis_list_t *dest);

axis_UTILS_API void axis_list_reverse(axis_list_t *src);
