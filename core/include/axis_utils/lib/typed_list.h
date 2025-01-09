//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include "aptima_utils/lib/typed_list_node.h"

#define aptima_TYPED_LIST_SIGNATURE 0xF77BB44C7D13991FU

#define aptima_typed_list_foreach(T, list, iter) \
  aptima_typed_list_foreach_(T, list, iter)
#define aptima_typed_list_foreach_(T, list, iter)                                 \
  for (aptima_typed_##T##_list_iterator_t iter =                                  \
           {                                                                   \
               NULL,                                                           \
               aptima_typed_##T##_list_front(list),                               \
               aptima_typed_##T##_list_front(list)                                \
                   ? aptima_typed_##T##_list_front(list)->next                    \
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
  typedef struct aptima_typed_##T##_list_t aptima_typed_##T##_list_t

// Because T is surrounded by ## in aptima_DEFINE_TYPED_LIST_(T), and that would
// prevent T from expanding itself, so the following aptima_DEFINE_TYPED_LIST() is
// to ensure T is expanded first before entering into aptima_DEFINE_TYPED_LIST_(T)
#define aptima_DEFINE_TYPED_LIST(T) aptima_DEFINE_TYPED_LIST_(T)
#define aptima_DEFINE_TYPED_LIST_(T)                                              \
  typedef struct aptima_typed_##T##_list_t {                                      \
    aptima_signature_t signature;                                                 \
    size_t size;                                                               \
    aptima_typed_##T##_list_node_t *front, *back;                                 \
  } aptima_typed_##T##_list_t;                                                    \
                                                                               \
  typedef struct aptima_typed_##T##_list_iterator_t {                             \
    aptima_typed_##T##_list_node_t *prev;                                         \
    aptima_typed_##T##_list_node_t *node;                                         \
    aptima_typed_##T##_list_node_t *next;                                         \
    size_t index;                                                              \
  } aptima_typed_##T##_list_iterator_t;                                           \
                                                                               \
  inline bool aptima_typed_##T##_list_check_integrity(                            \
      aptima_typed_##T##_list_t *self) {                                          \
    aptima_ASSERT(self, "Invalid argument.");                                     \
                                                                               \
    if (aptima_signature_get(&self->signature) != aptima_TYPED_LIST_SIGNATURE) {     \
      aptima_ASSERT(0, "Should not happen.");                                     \
      return false;                                                            \
    }                                                                          \
                                                                               \
    if (!self->size) {                                                         \
      if (self->front != NULL || self->back != NULL) {                         \
        aptima_ASSERT(0, "Should not happen.");                                   \
        return false;                                                          \
      }                                                                        \
    } else {                                                                   \
      if (self->front == NULL || self->back == NULL) {                         \
        aptima_ASSERT(0, "Should not happen.");                                   \
        return false;                                                          \
      }                                                                        \
                                                                               \
      if (self->size == 1) {                                                   \
        if (self->front != self->back) {                                       \
          aptima_ASSERT(0, "Should not happen.");                                 \
          return false;                                                        \
        }                                                                      \
        if ((self->front->prev != NULL) || (self->front->next != NULL)) {      \
          aptima_ASSERT(0, "Should not happen.");                                 \
          return false;                                                        \
        }                                                                      \
      }                                                                        \
                                                                               \
      if (self->front->prev != NULL) {                                         \
        aptima_ASSERT(0, "Should not happen.");                                   \
        return false;                                                          \
      }                                                                        \
      if (self->back->next != NULL) {                                          \
        aptima_ASSERT(0, "Should not happen.");                                   \
        return false;                                                          \
      }                                                                        \
    }                                                                          \
    return true;                                                               \
  }                                                                            \
                                                                               \
  inline aptima_typed_##T##_list_node_t *aptima_typed_##T##_list_front(              \
      aptima_typed_##T##_list_t *self) {                                          \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    return self->front;                                                        \
  }                                                                            \
                                                                               \
  inline aptima_typed_##T##_list_node_t *aptima_typed_##T##_list_back(               \
      aptima_typed_##T##_list_t *self) {                                          \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    return self->back;                                                         \
  }                                                                            \
                                                                               \
  inline void aptima_typed_##T##_list_init(aptima_typed_##T##_list_t *self) {        \
    aptima_ASSERT(self, "Invalid argument.");                                     \
                                                                               \
    aptima_signature_set(&self->signature, aptima_TYPED_LIST_SIGNATURE);             \
    self->size = 0;                                                            \
    self->front = NULL;                                                        \
    self->back = NULL;                                                         \
  }                                                                            \
                                                                               \
  inline void aptima_typed_##T##_list_clear(aptima_typed_##T##_list_t *self) {       \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    aptima_typed_list_foreach(T, self, iter) {                                    \
      aptima_typed_##T##_list_node_destroy(iter.node);                            \
    }                                                                          \
    self->size = 0;                                                            \
    self->front = NULL;                                                        \
    self->back = NULL;                                                         \
  }                                                                            \
                                                                               \
  inline void aptima_typed_##T##_list_deinit(aptima_typed_##T##_list_t *self) {      \
    aptima_typed_##T##_list_clear(self);                                          \
  }                                                                            \
                                                                               \
  inline aptima_typed_##T##_list_t *aptima_typed_##T##_list_create(void) {           \
    aptima_typed_##T##_list_t *self =                                             \
        (aptima_typed_##T##_list_t *)aptima_malloc(sizeof(aptima_typed_##T##_list_t));  \
    aptima_ASSERT(self, "Failed to allocate memory.");                            \
                                                                               \
    aptima_typed_##T##_list_init(self);                                           \
    return self;                                                               \
  }                                                                            \
                                                                               \
  inline void aptima_typed_##T##_list_destroy(aptima_typed_##T##_list_t *self) {     \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    aptima_typed_##T##_list_deinit(self);                                         \
    aptima_free(self);                                                            \
  }                                                                            \
                                                                               \
  inline size_t aptima_typed_##T##_list_size(aptima_typed_##T##_list_t *self) {      \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    return self->size;                                                         \
  }                                                                            \
                                                                               \
  inline bool aptima_typed_##T##_list_is_empty(aptima_typed_##T##_list_t *self) {    \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    return !aptima_typed_##T##_list_size(self);                                   \
  }                                                                            \
                                                                               \
  inline void aptima_typed_##T##_list_swap(aptima_typed_##T##_list_t *self,          \
                                        aptima_typed_##T##_list_t *target) {      \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(target, "Invalid argument.");                                   \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(target),                   \
               "Invalid argument.");                                           \
                                                                               \
    aptima_typed_##T##_list_node_t *self_front = self->front;                     \
    aptima_typed_##T##_list_node_t *self_back = self->back;                       \
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
  inline void aptima_typed_##T##_list_concat(aptima_typed_##T##_list_t *self,        \
                                          aptima_typed_##T##_list_t *target) {    \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(target, "Invalid argument.");                                   \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(target),                   \
               "Invalid argument.");                                           \
                                                                               \
    if (aptima_typed_##T##_list_size(target) == 0) {                              \
      return;                                                                  \
    }                                                                          \
                                                                               \
    if (aptima_typed_##T##_list_size(self) == 0) {                                \
      aptima_typed_##T##_list_swap(self, target);                                 \
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
  inline void aptima_typed_##T##_list_push_front(                                 \
      aptima_typed_##T##_list_t *self, aptima_typed_##T##_list_node_t *node) {       \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(node, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    if (aptima_typed_##T##_list_is_empty(self)) {                                 \
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
  inline void aptima_typed_##T##_list_push_list_node_back(                        \
      aptima_typed_##T##_list_t *self, aptima_typed_##T##_list_node_t *node) {       \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(node, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    if (aptima_typed_##T##_list_is_empty(self)) {                                 \
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
  inline void aptima_typed_##T##_list_push_back(                                  \
      aptima_typed_##T##_list_t *self, T item,                                    \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *)) {                                                 \
    aptima_typed_##T##_list_node_t *item_node =                                   \
        aptima_typed_list_node_create(T)(item, construct, move, copy, destruct);  \
    aptima_typed_##T##_list_push_list_node_back(self, item_node);                 \
  }                                                                            \
                                                                               \
  inline void aptima_typed_##T##_list_push_back_in_place(                         \
      aptima_typed_##T##_list_t *self, void *data,                                \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *)) {                                                 \
    aptima_typed_##T##_list_node_t *item_node =                                   \
        aptima_typed_list_node_create_in_place(T)(data, construct, move, copy,    \
                                               destruct);                      \
    aptima_typed_##T##_list_push_list_node_back(self, item_node);                 \
  }                                                                            \
                                                                               \
  inline aptima_typed_##T##_list_node_t *aptima_typed_##T##_list_pop_front(          \
      aptima_typed_##T##_list_t *self) {                                          \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    if (aptima_typed_##T##_list_is_empty(self)) {                                 \
      return NULL;                                                             \
    }                                                                          \
    aptima_typed_##T##_list_node_t *node = self->front;                           \
    if (aptima_typed_##T##_list_size(self) == 1) {                                \
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
  inline aptima_typed_##T##_list_node_t *aptima_typed_##T##_list_pop_back(           \
      aptima_typed_##T##_list_t *self) {                                          \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    if (aptima_typed_##T##_list_is_empty(self)) {                                 \
      return NULL;                                                             \
    }                                                                          \
    aptima_typed_##T##_list_node_t *node = self->back;                            \
    if (aptima_typed_##T##_list_size(self) == 1) {                                \
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
  inline void aptima_typed_##T##_list_copy(aptima_typed_##T##_list_t *self,          \
                                        aptima_typed_##T##_list_t *target) {      \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(target, "Invalid argument.");                                   \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(target),                   \
               "Invalid argument.");                                           \
                                                                               \
    aptima_typed_list_foreach(T, target, iter) {                                  \
      aptima_ASSERT(iter.node, "Invalid argument.");                              \
                                                                               \
      aptima_typed_list_node_t(T) *new_node =                                     \
          aptima_typed_list_node_clone(T)(iter.node);                             \
      aptima_typed_list_push_list_node_back(T)(self, new_node);                   \
    }                                                                          \
  }                                                                            \
                                                                               \
  inline aptima_typed_##T##_list_iterator_t aptima_typed_##T##_list_begin(           \
      aptima_typed_##T##_list_t *self) {                                          \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_check_integrity(self),                     \
               "Invalid argument.");                                           \
                                                                               \
    return (aptima_typed_##T##_list_iterator_t){                                  \
        NULL,                                                                  \
        aptima_typed_##T##_list_front(self),                                      \
        aptima_typed_##T##_list_front(self)                                       \
            ? aptima_typed_##T##_list_front(self)->next                           \
            : NULL,                                                            \
        0,                                                                     \
    };                                                                         \
  }                                                                            \
                                                                               \
  inline aptima_typed_list_node_t(T) *                                            \
      aptima_typed_##T##_list_find(aptima_typed_##T##_list_t *self, T *item,         \
                                bool (*compare)(const T *, const T *)) {       \
    aptima_ASSERT(                                                                \
        self && aptima_typed_##T##_list_check_integrity(self) && item && compare, \
        "Invalid argument.");                                                  \
    aptima_typed_list_foreach(T, self, iter) {                                    \
      if (compare(aptima_typed_list_node_get_data(T)(iter.node), item)) {         \
        return iter.node;                                                      \
      }                                                                        \
    }                                                                          \
    return NULL;                                                               \
  }                                                                            \
                                                                               \
  inline aptima_typed_##T##_list_iterator_t aptima_typed_##T##_list_iterator_next(   \
      aptima_typed_##T##_list_iterator_t self) {                                  \
    return (aptima_typed_##T##_list_iterator_t){                                  \
        self.node, self.next, self.node ? (self.node)->next : NULL,            \
        self.index + 1};                                                       \
  }                                                                            \
                                                                               \
  inline bool aptima_typed_##T##_list_iterator_is_end(                            \
      aptima_typed_##T##_list_iterator_t self) {                                  \
    return self.node == NULL;                                                  \
  }                                                                            \
  inline aptima_typed_##T##_list_node_t                                           \
      *aptima_typed_##T##_list_iterator_to_list_node(                             \
          aptima_typed_##T##_list_iterator_t self) {                              \
    return self.node;                                                          \
  }

#define aptima_TYPED_LIST_INIT_VAL           \
  {.signature = aptima_TYPED_LIST_SIGNATURE, \
   .size = 0,                             \
   .front = NULL,                         \
   .back = NULL}

#define aptima_DECLARE_TYPED_LIST_INLINE_ASSETS(T) \
  aptima_DECLARE_TYPED_LIST_INLINE_ASSETS_(T)
#define aptima_DECLARE_TYPED_LIST_INLINE_ASSETS_(T)                               \
  extern inline bool aptima_typed_##T##_list_check_integrity(                     \
      aptima_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline aptima_typed_##T##_list_node_t *aptima_typed_##T##_list_front(       \
      aptima_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline aptima_typed_##T##_list_node_t *aptima_typed_##T##_list_back(        \
      aptima_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline void aptima_typed_##T##_list_init(aptima_typed_##T##_list_t *self);  \
                                                                               \
  extern inline void aptima_typed_##T##_list_clear(aptima_typed_##T##_list_t *self); \
                                                                               \
  extern inline void aptima_typed_##T##_list_deinit(                              \
      aptima_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline aptima_typed_##T##_list_t *aptima_typed_##T##_list_create(void);     \
                                                                               \
  extern inline void aptima_typed_##T##_list_destroy(                             \
      aptima_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline size_t aptima_typed_##T##_list_size(                              \
      aptima_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline bool aptima_typed_##T##_list_is_empty(                            \
      aptima_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline void aptima_typed_##T##_list_swap(                                \
      aptima_typed_##T##_list_t *self, aptima_typed_##T##_list_t *target);           \
                                                                               \
  extern inline void aptima_typed_##T##_list_concat(                              \
      aptima_typed_##T##_list_t *self, aptima_typed_##T##_list_t *target);           \
                                                                               \
  extern inline void aptima_typed_##T##_list_push_front(                          \
      aptima_typed_##T##_list_t *self, aptima_typed_##T##_list_node_t *node);        \
                                                                               \
  extern inline void aptima_typed_##T##_list_push_list_node_back(                 \
      aptima_typed_##T##_list_t *self, aptima_typed_##T##_list_node_t *node);        \
                                                                               \
  extern inline void aptima_typed_##T##_list_push_back(                           \
      aptima_typed_##T##_list_t *self, T item,                                    \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *));         /* NOLINT */                             \
                                                                               \
  extern inline void aptima_typed_##T##_list_push_back_in_place(                  \
      aptima_typed_##T##_list_t *self, void *data,                                \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *));         /* NOLINT */                             \
                                                                               \
  extern inline aptima_typed_##T##_list_node_t *aptima_typed_##T##_list_pop_front(   \
      aptima_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline aptima_typed_##T##_list_node_t *aptima_typed_##T##_list_pop_back(    \
      aptima_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline void aptima_typed_##T##_list_copy(                                \
      aptima_typed_##T##_list_t *self, aptima_typed_##T##_list_t *target);           \
                                                                               \
  extern inline aptima_typed_list_node_t(T) *                                     \
      aptima_typed_##T##_list_find(aptima_typed_##T##_list_t *self, T *item,         \
                                bool (*compare)(const T *, const T *));        \
                                                                               \
  extern inline aptima_typed_##T##_list_iterator_t aptima_typed_##T##_list_begin(    \
      aptima_typed_##T##_list_t *self);                                           \
                                                                               \
  extern inline aptima_typed_##T##_list_iterator_t                                \
      aptima_typed_##T##_list_iterator_next(                                      \
          aptima_typed_##T##_list_iterator_t self);                               \
                                                                               \
  extern inline bool aptima_typed_##T##_list_iterator_is_end(                     \
      aptima_typed_##T##_list_iterator_t self);                                   \
                                                                               \
  extern inline aptima_typed_##T##_list_node_t                                    \
      *aptima_typed_##T##_list_iterator_to_list_node(                             \
          aptima_typed_##T##_list_iterator_t self);

#define aptima_typed_list_t(T) aptima_typed_list_t_(T)
#define aptima_typed_list_t_(T) aptima_typed_##T##_list_t

#define aptima_typed_list_size(T) aptima_typed_list_size_(T)
#define aptima_typed_list_size_(T) aptima_typed_##T##_list_size

#define aptima_typed_list_init(T) aptima_typed_list_init_(T)
#define aptima_typed_list_init_(T) aptima_typed_##T##_list_init

#define aptima_typed_list_deinit(T) aptima_typed_list_deinit_(T)
#define aptima_typed_list_deinit_(T) aptima_typed_##T##_list_deinit

#define aptima_typed_list_create(T) aptima_typed_list_create_(T)
#define aptima_typed_list_create_(T) aptima_typed_##T##_list_create

#define aptima_typed_list_destroy(T) aptima_typed_list_destroy_(T)
#define aptima_typed_list_destroy_(T) aptima_typed_##T##_list_destroy

#define aptima_typed_list_clear(T) aptima_typed_list_clear_(T)
#define aptima_typed_list_clear_(T) aptima_typed_##T##_list_clear

#define aptima_typed_list_copy(T) aptima_typed_list_copy_(T)
#define aptima_typed_list_copy_(T) aptima_typed_##T##_list_copy

#define aptima_typed_list_find(T) aptima_typed_list_find_(T)
#define aptima_typed_list_find_(T) aptima_typed_##T##_list_find

#define aptima_typed_list_front(T) aptima_typed_list_front_(T)
#define aptima_typed_list_front_(T) aptima_typed_##T##_list_front

#define aptima_typed_list_pop_front(T) aptima_typed_list_pop_front_(T)
#define aptima_typed_list_pop_front_(T) aptima_typed_##T##_list_pop_front

#define aptima_typed_list_back(T) aptima_typed_list_back_(T)
#define aptima_typed_list_back_(T) aptima_typed_##T##_list_back

#define aptima_typed_list_push_list_node_back(T) \
  aptima_typed_list_push_list_node_back_(T)
#define aptima_typed_list_push_list_node_back_(T) \
  aptima_typed_##T##_list_push_list_node_back

#define aptima_typed_list_push_back(T) aptima_typed_list_push_back_(T)
#define aptima_typed_list_push_back_(T) aptima_typed_##T##_list_push_back

#define aptima_typed_list_push_back_in_place(T) \
  aptima_typed_list_push_back_in_place_(T)
#define aptima_typed_list_push_back_in_place_(T) \
  aptima_typed_##T##_list_push_back_in_place

#define aptima_typed_list_swap(T) aptima_typed_list_swap_(T)
#define aptima_typed_list_swap_(T) aptima_typed_##T##_list_swap

#define aptima_typed_list_is_empty(T) aptima_typed_list_is_empty_(T)
#define aptima_typed_list_is_empty_(T) aptima_typed_##T##_list_is_empty

#define aptima_typed_list_iterator_t(T) aptima_typed_list_iterator_t_(T)
#define aptima_typed_list_iterator_t_(T) aptima_typed_##T##_list_iterator_t

#define aptima_typed_list_begin(T) aptima_typed_list_begin_(T)
#define aptima_typed_list_begin_(T) aptima_typed_##T##_list_begin

#define aptima_typed_list_iterator_next(T) aptima_typed_list_iterator_next_(T)
#define aptima_typed_list_iterator_next_(T) aptima_typed_##T##_list_iterator_next
