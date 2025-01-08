//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/lib/typed_list_node.h"

#define axis_TYPED_LIST_SIGNATURE 0xF77BB44C7D13991FU

#define axis_typed_list_foreach(T, list, iter) \
  axis_typed_list_foreach_(T, list, iter)
#define axis_typed_list_foreach_(T, list, iter)                                 \
  for (axis_typed_##T##_list_iterator_t iter =                                  \
           {                                                                   \
               NULL,                                                           \
               axis_typed_##T##_list_front(list),                               \
               axis_typed_##T##_list_front(list)                                \
                   ? axis_typed_##T##_list_front(list)->next                    \
                   : NULL,                                                     \
               0,                                                              \
           };                                                                  \
       (iter).node;                                                            \
       ++((iter).index), (iter).prev = (iter).node, (iter).node = (iter).next, \
                                       (iter).next = (iter).node               \
                                                         ? ((iter).node)->next \
                                                         : NULL)

#define DECLARE_LIST(T) DECLARE_LIST_(T)
#define DECLARE_LIST_(T) \
  typedef struct axis_typed_##T##_list_t axis_typed_##T##_list_t

// Because T is surrounded by ## in axis_DEFINE_TYPED_LIST_(T), and that would
// prevent T from expanding itself, so the following axis_DEFINE_TYPED_LIST() is
// to ensure T is expanded first before entering into axis_DEFINE_TYPED_LIST_(T)
#define axis_DEFINE_TYPED_LIST(T) axis_DEFINE_TYPED_LIST_(T)
#define axis_DEFINE_TYPED_LIST_(T)                                              \
  typedef struct axis_typed_##T##_list_t {                                      \
    axis_signature_t signature;                                                 \
    size_t size;                                                               \
    axis_typed_##T##_list_node_t *front, *back;                                 \
  } axis_typed_##T##_list_t;                                                    \
                                                                               \
  typedef struct axis_typed_##T##_list_iterator_t {                             \
    axis_typed_##T##_list_node_t *prev;                                         \
    axis_typed_##T##_list_node_t *node;                                         \
    axis_typed_##T##_list_node_t *next;                                         \
    size_t index;                                                              \
  } axis_typed_##T##_list_iterator_t;                                           \
                                                                               \
  inline bool axis_typed_##T##_list_check_integrity(                            \
      axis_typed_##T##_list_t *self) {                                          \
    axis_ASSERT(self, "Invalid argument.");                                     \
                                                                               \
    if (axis_signature_get(&self->signature) != axis_TYPED_LIST_SIGNATURE) {     \
      axis_ASSERT(0, "Should not happen.");                                     \
      return false;                                                            \
    }                                                                          \
                                                                               \
    if (!self->size) {                                                         \
      if (self->front != NULL || self->back != NULL) {                         \
        axis_ASSERT(0, "Should not happen.");                                   \
        return false;                                                          \
      }                                                                        \
    } else {                                                                   \
      if (self->front == NULL || self->back == NULL) {                         \
        axis_ASSERT(0, "Should not happen.");                                   \
        return false;                                                          \
      }                                                                        \
                                                                               \
      if (self->size == 1) {                                                   \
        if (self->front != self->back) {                                       \
          axis_ASSERT(0, "Should not happen.");                                 \
          return false;                                                        \
        }                                                                      \
        if ((self->front->prev != NULL) || (self->front->next != NULL)) {      \
          axis_ASSERT(0, "Should not happen.");                                 \
          return false;                                                        \
        }                                                                      \
      }                                                                        \
                                                                               \
      if (self->front->prev != NULL) {                                         \
        axis_ASSERT(0, "Should not happen.");                                   \
        return false;                                                          \
      }                                                                        \
      if (self->back->next != NULL) {                                          \
        axis_ASSERT(0, "Should not happen.");                                   \
        return false;                                                          \
      }                                                                        \
    }                                                                          \
    return true;                                                               \
  }                                                                            \
                                                                               \
  inline axis_typed_##T##_list_node_t *axis_typed_##T##_list_front(              \
      axis_typed_##T##_list_t *self) {                                          \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    return self->front;                                                        \
  }                                                                            \
                                                                               \
  inline axis_typed_##T##_list_node_t *axis_typed_##T##_list_back(               \
      axis_typed_##T##_list_t *self) {                                          \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    return self->back;                                                         \
  }                                                                            \
                                                                               \
  inline void axis_typed_##T##_list_init(axis_typed_##T##_list_t *self) {        \
    axis_ASSERT(self, "Invalid argument.");                                     \
                                                                               \
    axis_signature_set(&self->signature, axis_TYPED_LIST_SIGNATURE);             \
    self->size = 0;                                                            \
    self->front = NULL;                                                        \
    self->back = NULL;                                                         \
  }                                                                            \
                                                                               \
  inline void axis_typed_##T##_list_clear(axis_typed_##T##_list_t *self) {       \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    axis_typed_list_foreach(T, self, iter) {                                    \
      axis_typed_##T##_list_node_destroy(iter.node);                            \
    }                                                                          \
    self->size = 0;                                                            \
    self->front = NULL;                                                        \
    self->back = NULL;                                                         \
  }                                                                            \
                                                                               \
  inline void axis_typed_##T##_list_deinit(axis_typed_##T##_list_t *self) {      \
    axis_typed_##T##_list_clear(self);                                          \
  }                                                                            \
                                                                               \
  inline axis_typed_##T##_list_t *axis_typed_##T##_list_create(void) {           \
    axis_typed_##T##_list_t *self =                                             \
        (axis_typed_##T##_list_t *)axis_malloc(sizeof(axis_typed_##T##_list_t));  \
    axis_ASSERT(self, "Failed to allocate memory.");                            \
                                                                               \
    axis_typed_##T##_list_init(self);                                           \
    return self;                                                               \
  }                                                                            \
                                                                               \
  inline void axis_typed_##T##_list_destroy(axis_typed_##T##_list_t *self) {     \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    axis_typed_##T##_list_deinit(self);                                         \
    axis_free(self);                                                            \
  }                                                                            \
                                                                               \
  inline size_t axis_typed_##T##_list_size(axis_typed_##T##_list_t *self) {      \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    return self->size;                                                         \
  }                                                                            \
                                                                               \
  inline bool axis_typed_##T##_list_is_empty(axis_typed_##T##_list_t *self) {    \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    return !axis_typed_##T##_list_size(self);                                   \
  }                                                                            \
                                                                               \
  inline void axis_typed_##T##_list_swap(axis_typed_##T##_list_t *self,          \
                                        axis_typed_##T##_list_t *target) {      \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(target, "Invalid argument.");                                   \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(target),                   \
               "Invalid argument.");                                           \
                                                                               \
    axis_typed_##T##_list_node_t *self_front = self->front;                     \
    axis_typed_##T##_list_node_t *self_back = self->back;                       \
    size_t self_size = self->size;                                             \
                                                                               \
    self->front = target->front;                                               \
    self->back = target->back;                                                 \
    self->size = target->size;                                                 \
                                                                               \
    target->front = self_front;                                                \
    target->back = self_back;                                                  \
    target->size = self_size;                                                  \
  }                                                                            \
                                                                               \
  inline void axis_typed_##T##_list_concat(axis_typed_##T##_list_t *self,        \
                                          axis_typed_##T##_list_t *target) {    \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(target, "Invalid argument.");                                   \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(target),                   \
               "Invalid argument.");                                           \
                                                                               \
    if (axis_typed_##T##_list_size(target) == 0) {                              \
      return;                                                                  \
    }                                                                          \
                                                                               \
    if (axis_typed_##T##_list_size(self) == 0) {                                \
      axis_typed_##T##_list_swap(self, target);                                 \
      return;                                                                  \
    }                                                                          \
                                                                               \
    self->back->next = target->front;                                          \
    target->front->prev = self->back;                                          \
    self->back = target->back;                                                 \
    self->size += target->size;                                                \
                                                                               \
    target->front = NULL;                                                      \
    target->back = NULL;                                                       \
    target->size = 0;                                                          \
  }                                                                            \
                                                                               \
  inline void axis_typed_##T##_list_push_front(                                 \
      axis_typed_##T##_list_t *self, axis_typed_##T##_list_node_t *node) {       \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(node, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    if (axis_typed_##T##_list_is_empty(self)) {                                 \
      self->back = self->front = node;                                         \
      node->prev = node->next = NULL;                                          \
    } else {                                                                   \
      node->next = self->front;                                                \
      node->prev = NULL;                                                       \
                                                                               \
      self->front->prev = node;                                                \
      self->front = node;                                                      \
    }                                                                          \
    ++self->size;                                                              \
  }                                                                            \
                                                                               \
  inline void axis_typed_##T##_list_push_list_node_back(                        \
      axis_typed_##T##_list_t *self, axis_typed_##T##_list_node_t *node) {       \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(node, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    if (axis_typed_##T##_list_is_empty(self)) {                                 \
      self->front = self->back = node;                                         \
      node->next = node->prev = NULL;                                          \
    } else {                                                                   \
      node->next = NULL;                                                       \
      node->prev = self->back;                                                 \
                                                                               \
      self->back->next = node;                                                 \
      self->back = node;                                                       \
    }                                                                          \
    ++self->size;                                                              \
  }                                                                            \
                                                                               \
  inline void axis_typed_##T##_list_push_back(                                  \
      axis_typed_##T##_list_t *self, T item,                                    \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *)) {                                                 \
    axis_typed_##T##_list_node_t *item_node =                                   \
        axis_typed_list_node_create(T)(item, construct, move, copy, destruct);  \
    axis_typed_##T##_list_push_list_node_back(self, item_node);                 \
  }                                                                            \
                                                                               \
  inline void axis_typed_##T##_list_push_back_in_place(                         \
      axis_typed_##T##_list_t *self, void *data,                                \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *)) {                                                 \
    axis_typed_##T##_list_node_t *item_node =                                   \
        axis_typed_list_node_create_in_place(T)(data, construct, move, copy,    \
                                               destruct);                      \
    axis_typed_##T##_list_push_list_node_back(self, item_node);                 \
  }                                                                            \
                                                                               \
  inline axis_typed_##T##_list_node_t *axis_typed_##T##_list_pop_front(          \
      axis_typed_##T##_list_t *self) {                                          \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    if (axis_typed_##T##_list_is_empty(self)) {                                 \
      return NULL;                                                             \
    }                                                                          \
    axis_typed_##T##_list_node_t *node = self->front;                           \
    if (axis_typed_##T##_list_size(self) == 1) {                                \
      self->back = self->front = NULL;                                         \
      node->prev = node->next = NULL;                                          \
    } else {                                                                   \
      self->front = self->front->next;                                         \
      self->front->prev = NULL;                                                \
                                                                               \
      node->next = NULL;                                                       \
    }                                                                          \
    --self->size;                                                              \
                                                                               \
    return node;                                                               \
  }                                                                            \
                                                                               \
  inline axis_typed_##T##_list_node_t *axis_typed_##T##_list_pop_back(           \
      axis_typed_##T##_list_t *self) {                                          \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    if (axis_typed_##T##_list_is_empty(self)) {                                 \
      return NULL;                                                             \
    }                                                                          \
    axis_typed_##T##_list_node_t *node = self->back;                            \
    if (axis_typed_##T##_list_size(self) == 1) {                                \
      self->front = self->back = NULL;                                         \
      node->prev = node->next = NULL;                                          \
    } else {                                                                   \
      self->back = self->back->prev;                                           \
      self->back->next = NULL;                                                 \
                                                                               \
      node->prev = NULL;                                                       \
    }                                                                          \
    --self->size;                                                              \
                                                                               \
    return node;                                                               \
  }                                                                            \
                                                                               \
  inline void axis_typed_##T##_list_copy(axis_typed_##T##_list_t *self,          \
                                        axis_typed_##T##_list_t *target) {      \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(target, "Invalid argument.");                                   \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(target),                   \
               "Invalid argument.");                                           \
                                                                               \
    axis_typed_list_foreach(T, target, iter) {                                  \
      axis_ASSERT(iter.node, "Invalid argument.");                              \
                                                                               \
      axis_typed_list_node_t(T) *new_node =                                     \
          axis_typed_list_node_clone(T)(iter.node);                             \
      axis_typed_list_push_list_node_back(T)(self, new_node);                   \
    }                                                                          \
  }                                                                            \
                                                                               \
  inline axis_typed_##T##_list_iterator_t axis_typed_##T##_list_begin(           \
      axis_typed_##T##_list_t *self) {                                          \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    return (axis_typed_##T##_list_iterator_t){                                  \
        NULL,                                                                  \
        axis_typed_##T##_list_front(self),                                      \
        axis_typed_##T##_list_front(self)                                       \
            ? axis_typed_##T##_list_front(self)->next                           \
            : NULL,                                                            \
        0,                                                                     \
    };                                                                         \
  }                                                                            \
                                                                               \
  inline axis_typed_list_node_t(T) *                                            \
      axis_typed_##T##_list_find(axis_typed_##T##_list_t *self, T *item,         \
                                bool (*compare)(const T *, const T *)) {       \
    axis_ASSERT(                                                                \
        self && axis_typed_##T##_list_check_integrity(self) && item && compare, \
        "Invalid argument.");                                                  \
    axis_typed_list_foreach(T, self, iter) {                                    \
      if (compare(axis_typed_list_node_get_data(T)(iter.node), item)) {         \
        return iter.node;                                                      \
      }                                                                        \
    }                                                                          \
    return NULL;                                                               \
  }                                                                            \
                                                                               \
  inline axis_typed_##T##_list_iterator_t axis_typed_##T##_list_iterator_next(   \
      axis_typed_##T##_list_iterator_t self) {                                  \
    return (axis_typed_##T##_list_iterator_t){                                  \
        self.node, self.next, self.node ? (self.node)->next : NULL,            \
        self.index + 1};                                                       \
  }                                                                            \
                                                                               \
  inline bool axis_typed_##T##_list_iterator_is_end(                            \
      axis_typed_##T##_list_iterator_t self) {                                  \
    return self.node == NULL;                                                  \
  }                                                                            \
  inline axis_typed_##T##_list_node_t                                           \
      *axis_typed_##T##_list_iterator_to_list_node(                             \
          axis_typed_##T##_list_iterator_t self) {                              \
    return self.node;                                                          \
  }

#define axis_TYPED_LIST_INIT_VAL           \
  {.signature = axis_TYPED_LIST_SIGNATURE, \
   .size = 0,                             \
   .front = NULL,                         \
   .back = NULL}

#define axis_DECLARE_TYPED_LIST_INLINE_ASSETS(T) \
  axis_DECLARE_TYPED_LIST_INLINE_ASSETS_(T)
#define axis_DECLARE_TYPED_LIST_INLINE_ASSETS_(T)                               \
  extern inline bool axis_typed_##T##_list_check_integrity(                     \
      axis_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline axis_typed_##T##_list_node_t *axis_typed_##T##_list_front(       \
      axis_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline axis_typed_##T##_list_node_t *axis_typed_##T##_list_back(        \
      axis_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline void axis_typed_##T##_list_init(axis_typed_##T##_list_t *self);  \
                                                                               \
  extern inline void axis_typed_##T##_list_clear(axis_typed_##T##_list_t *self); \
                                                                               \
  extern inline void axis_typed_##T##_list_deinit(                              \
      axis_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline axis_typed_##T##_list_t *axis_typed_##T##_list_create(void);     \
                                                                               \
  extern inline void axis_typed_##T##_list_destroy(                             \
      axis_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline size_t axis_typed_##T##_list_size(                              \
      axis_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline bool axis_typed_##T##_list_is_empty(                            \
      axis_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline void axis_typed_##T##_list_swap(                                \
      axis_typed_##T##_list_t *self, axis_typed_##T##_list_t *target);           \
                                                                               \
  extern inline void axis_typed_##T##_list_concat(                              \
      axis_typed_##T##_list_t *self, axis_typed_##T##_list_t *target);           \
                                                                               \
  extern inline void axis_typed_##T##_list_push_front(                          \
      axis_typed_##T##_list_t *self, axis_typed_##T##_list_node_t *node);        \
                                                                               \
  extern inline void axis_typed_##T##_list_push_list_node_back(                 \
      axis_typed_##T##_list_t *self, axis_typed_##T##_list_node_t *node);        \
                                                                               \
  extern inline void axis_typed_##T##_list_push_back(                           \
      axis_typed_##T##_list_t *self, T item,                                    \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *));         /* NOLINT */                             \
                                                                               \
  extern inline void axis_typed_##T##_list_push_back_in_place(                  \
      axis_typed_##T##_list_t *self, void *data,                                \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *));         /* NOLINT */                             \
                                                                               \
  extern inline axis_typed_##T##_list_node_t *axis_typed_##T##_list_pop_front(   \
      axis_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline axis_typed_##T##_list_node_t *axis_typed_##T##_list_pop_back(    \
      axis_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline void axis_typed_##T##_list_copy(                                \
      axis_typed_##T##_list_t *self, axis_typed_##T##_list_t *target);           \
                                                                               \
  extern inline axis_typed_list_node_t(T) *                                     \
      axis_typed_##T##_list_find(axis_typed_##T##_list_t *self, T *item,         \
                                bool (*compare)(const T *, const T *));        \
                                                                               \
  extern inline axis_typed_##T##_list_iterator_t axis_typed_##T##_list_begin(    \
      axis_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline axis_typed_##T##_list_iterator_t                                \
      axis_typed_##T##_list_iterator_next(                                      \
          axis_typed_##T##_list_iterator_t self);                               \
                                                                               \
  extern inline bool axis_typed_##T##_list_iterator_is_end(                     \
      axis_typed_##T##_list_iterator_t self);                                   \
                                                                               \
  extern inline axis_typed_##T##_list_node_t                                    \
      *axis_typed_##T##_list_iterator_to_list_node(                             \
          axis_typed_##T##_list_iterator_t self);

#define axis_typed_list_t(T) axis_typed_list_t_(T)
#define axis_typed_list_t_(T) axis_typed_##T##_list_t

#define axis_typed_list_size(T) axis_typed_list_size_(T)
#define axis_typed_list_size_(T) axis_typed_##T##_list_size

#define axis_typed_list_init(T) axis_typed_list_init_(T)
#define axis_typed_list_init_(T) axis_typed_##T##_list_init

#define axis_typed_list_deinit(T) axis_typed_list_deinit_(T)
#define axis_typed_list_deinit_(T) axis_typed_##T##_list_deinit

#define axis_typed_list_create(T) axis_typed_list_create_(T)
#define axis_typed_list_create_(T) axis_typed_##T##_list_create

#define axis_typed_list_destroy(T) axis_typed_list_destroy_(T)
#define axis_typed_list_destroy_(T) axis_typed_##T##_list_destroy

#define axis_typed_list_clear(T) axis_typed_list_clear_(T)
#define axis_typed_list_clear_(T) axis_typed_##T##_list_clear

#define axis_typed_list_copy(T) axis_typed_list_copy_(T)
#define axis_typed_list_copy_(T) axis_typed_##T##_list_copy

#define axis_typed_list_find(T) axis_typed_list_find_(T)
#define axis_typed_list_find_(T) axis_typed_##T##_list_find

#define axis_typed_list_front(T) axis_typed_list_front_(T)
#define axis_typed_list_front_(T) axis_typed_##T##_list_front

#define axis_typed_list_pop_front(T) axis_typed_list_pop_front_(T)
#define axis_typed_list_pop_front_(T) axis_typed_##T##_list_pop_front

#define axis_typed_list_back(T) axis_typed_list_back_(T)
#define axis_typed_list_back_(T) axis_typed_##T##_list_back

#define axis_typed_list_push_list_node_back(T) \
  axis_typed_list_push_list_node_back_(T)
#define axis_typed_list_push_list_node_back_(T) \
  axis_typed_##T##_list_push_list_node_back

#define axis_typed_list_push_back(T) axis_typed_list_push_back_(T)
#define axis_typed_list_push_back_(T) axis_typed_##T##_list_push_back

#define axis_typed_list_push_back_in_place(T) \
  axis_typed_list_push_back_in_place_(T)
#define axis_typed_list_push_back_in_place_(T) \
  axis_typed_##T##_list_push_back_in_place

#define axis_typed_list_swap(T) axis_typed_list_swap_(T)
#define axis_typed_list_swap_(T) axis_typed_##T##_list_swap

#define axis_typed_list_is_empty(T) axis_typed_list_is_empty_(T)
#define axis_typed_list_is_empty_(T) axis_typed_##T##_list_is_empty

#define axis_typed_list_iterator_t(T) axis_typed_list_iterator_t_(T)
#define axis_typed_list_iterator_t_(T) axis_typed_##T##_list_iterator_t

#define axis_typed_list_begin(T) axis_typed_list_begin_(T)
#define axis_typed_list_begin_(T) axis_typed_##T##_list_begin

#define axis_typed_list_iterator_next(T) axis_typed_list_iterator_next_(T)
#define axis_typed_list_iterator_next_(T) axis_typed_##T##_list_iterator_next
