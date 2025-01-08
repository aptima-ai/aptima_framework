//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/macro/check.h"

#define axis_TYPED_LIST_NODE_SIGNATURE 0x3CE1EAC77F72D345U

/**
 * @brief Because T is surrounded by ## in axis_DEFINE_TYPED_LIST_NODE_(T), and
 * that would prevent T from expanding itself, so the following
 * axis_DEFINE_TYPED_LIST_NODE() is to ensure T is expanded first before entering
 * into axis_DEFINE_TYPED_LIST_NODE_(T)
 *
 * @param T The type of the value stored in the list node.
 */
#define axis_DEFINE_TYPED_LIST_NODE(T) axis_DEFINE_TYPED_LIST_NODE_(T)
#define axis_DEFINE_TYPED_LIST_NODE_(T)                                         \
  typedef struct axis_typed_##T##_list_node_t axis_typed_##T##_list_node_t;      \
  struct axis_typed_##T##_list_node_t {                                         \
    axis_signature_t signature;                                                 \
    axis_typed_##T##_list_node_t *next, *prev;                                  \
    T data;                                                                    \
    void (*construct)(T *, void *); /* NOLINT */                               \
    void (*move)(T *, T *);         /* NOLINT */                               \
    void (*copy)(T *, T *);         /* NOLINT */                               \
    void (*destruct)(T *);          /* NOLINT */                               \
  };                                                                           \
                                                                               \
  inline bool axis_typed_##T##_list_node_check_integrity(                       \
      axis_typed_##T##_list_node_t *self) {                                     \
    axis_ASSERT(self, "Invalid argument.");                                     \
                                                                               \
    if (axis_signature_get(&self->signature) !=                                 \
        axis_TYPED_LIST_NODE_SIGNATURE) {                                       \
      axis_ASSERT(0, "Should not happen.");                                     \
      return false;                                                            \
    }                                                                          \
    return true;                                                               \
  }                                                                            \
                                                                               \
  inline bool axis_typed_##T##_list_node_init_empty(                            \
      axis_typed_##T##_list_node_t *self, void (*construct)(T *, void *),       \
      void (*move)(T *, T *),  /* NOLINT */                                    \
      void (*copy)(T *, T *),  /* NOLINT */                                    \
      void (*destruct)(T *)) { /* NOLINT */                                    \
    axis_ASSERT(self, "Invalid argument.");                                     \
                                                                               \
    axis_signature_set(&self->signature, axis_TYPED_LIST_NODE_SIGNATURE);        \
    self->next = NULL;                                                         \
    self->prev = NULL;                                                         \
                                                                               \
    self->construct = construct;                                               \
    self->move = move;                                                         \
    self->copy = copy;                                                         \
    self->destruct = destruct;                                                 \
                                                                               \
    return true;                                                               \
  }                                                                            \
                                                                               \
  inline bool axis_typed_##T##_list_node_init(                                  \
      axis_typed_##T##_list_node_t *self, T data,                               \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *)) {        /* NOLINT */                             \
    axis_typed_##T##_list_node_init_empty(self, construct, move, copy,          \
                                         destruct);                            \
    self->data = data;                                                         \
    return true;                                                               \
  }                                                                            \
                                                                               \
  inline bool axis_typed_##T##_list_node_init_in_place(                         \
      axis_typed_##T##_list_node_t *self, void *data,                           \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *)) {        /* NOLINT */                             \
    axis_typed_##T##_list_node_init_empty(self, construct, move, copy,          \
                                         destruct);                            \
    if (construct) {                                                           \
      construct(&self->data, data);                                            \
    }                                                                          \
    return true;                                                               \
  }                                                                            \
                                                                               \
  inline axis_typed_##T##_list_node_t *axis_typed_##T##_list_node_create_empty(  \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *)) {        /* NOLINT */                             \
    axis_typed_##T##_list_node_t *self =                                        \
        (axis_typed_##T##_list_node_t *)axis_malloc(                             \
            sizeof(axis_typed_##T##_list_node_t));                              \
    axis_ASSERT(self, "Failed to allocate memory.");                            \
                                                                               \
    axis_typed_##T##_list_node_init_empty(self, construct, move, copy,          \
                                         destruct);                            \
                                                                               \
    return self;                                                               \
  }                                                                            \
                                                                               \
  inline axis_typed_##T##_list_node_t *axis_typed_##T##_list_node_create(        \
      T data, void (*construct)(T *, void *), /* NOLINT */                     \
      void (*move)(T *, T *),                 /* NOLINT */                     \
      void (*copy)(T *, T *),                 /* NOLINT */                     \
      void (*destruct)(T *)) {                /* NOLINT */                     \
    axis_typed_##T##_list_node_t *self =                                        \
        (axis_typed_##T##_list_node_t *)axis_malloc(                             \
            sizeof(axis_typed_##T##_list_node_t));                              \
    axis_ASSERT(self, "Failed to allocate memory.");                            \
                                                                               \
    axis_typed_##T##_list_node_init(self, data, construct, move, copy,          \
                                   destruct);                                  \
    return self;                                                               \
  }                                                                            \
                                                                               \
  inline axis_typed_##T##_list_node_t                                           \
      *axis_typed_##T##_list_node_create_in_place(                              \
          void *data, void (*construct)(T *, void *), /* NOLINT */             \
          void (*move)(T *, T *),                     /* NOLINT */             \
          void (*copy)(T *, T *),                     /* NOLINT */             \
          void (*destruct)(T *)) {                    /* NOLINT */             \
    axis_typed_##T##_list_node_t *self =                                        \
        (axis_typed_##T##_list_node_t *)axis_malloc(                             \
            sizeof(axis_typed_##T##_list_node_t));                              \
    axis_ASSERT(self, "Failed to allocate memory.");                            \
                                                                               \
    axis_typed_##T##_list_node_init_in_place(self, data, construct, move, copy, \
                                            destruct);                         \
    return self;                                                               \
  }                                                                            \
                                                                               \
  inline axis_typed_##T##_list_node_t *axis_typed_##T##_list_node_clone(         \
      axis_typed_##T##_list_node_t *src) {                                      \
    axis_ASSERT(src, "Invalid argument.");                                      \
                                                                               \
    /* NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast) */               \
    axis_typed_##T##_list_node_t *self =                                        \
        (axis_typed_##T##_list_node_t *)axis_malloc(                             \
            sizeof(axis_typed_##T##_list_node_t));                              \
    axis_ASSERT(self, "Failed to allocate memory.");                            \
                                                                               \
    axis_typed_##T##_list_node_init_empty(self, src->construct, src->move,      \
                                         src->copy, src->destruct);            \
    src->copy(&self->data, &src->data);                                        \
    return self;                                                               \
  }                                                                            \
                                                                               \
  inline bool axis_typed_##T##_list_node_deinit(                                \
      axis_typed_##T##_list_node_t *self) {                                     \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_node_check_integrity(self),                \
               "Invalid argument.");                                           \
                                                                               \
    if (self->destruct) {                                                      \
      self->destruct(&(self->data));                                           \
    }                                                                          \
    return true;                                                               \
  }                                                                            \
                                                                               \
  inline bool axis_typed_##T##_list_node_destroy(                               \
      axis_typed_##T##_list_node_t *self) {                                     \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_node_check_integrity(self),                \
               "Invalid argument.");                                           \
                                                                               \
    if (!axis_typed_##T##_list_node_deinit(self)) {                             \
      return false;                                                            \
    }                                                                          \
    axis_free(self);                                                            \
    return true;                                                               \
  }                                                                            \
                                                                               \
  /* NOLINTNEXTLINE */                                                         \
  inline T *axis_typed_##T##_list_node_get_data(                                \
      axis_typed_##T##_list_node_t *self) {                                     \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_node_check_integrity(self),                \
               "Invalid argument.");                                           \
                                                                               \
    return &self->data;                                                        \
  }                                                                            \
                                                                               \
  inline bool axis_typed_##T##_list_node_set_data(                              \
      axis_typed_##T##_list_node_t *self, T *data, bool move) { /* NOLINT */    \
    axis_ASSERT(self, "Invalid argument.");                                     \
    axis_ASSERT(axis_typed_##T##_list_node_check_integrity(self),                \
               "Invalid argument.");                                           \
                                                                               \
    /* Destruct the old data if any */                                         \
    if (self->destruct) {                                                      \
      self->destruct(&self->data);                                             \
    }                                                                          \
    if (move) {                                                                \
      if (self->move) {                                                        \
        self->move(&self->data, data);                                         \
      } else {                                                                 \
        self->data = *data;                                                    \
      }                                                                        \
    } else {                                                                   \
      if (self->copy) {                                                        \
        self->copy(&self->data, data);                                         \
      } else {                                                                 \
        self->data = *data;                                                    \
      }                                                                        \
    }                                                                          \
    return true;                                                               \
  }

#define axis_DECLARE_TYPED_LIST_NODE_INLINE_ASSETS(T) \
  axis_DECLARE_TYPED_LIST_NODE_INLINE_ASSETS_(T)
#define axis_DECLARE_TYPED_LIST_NODE_INLINE_ASSETS_(T)                          \
  extern inline bool axis_typed_##T##_list_node_check_integrity(                \
      axis_typed_##T##_list_node_t *self);                                      \
                                                                               \
  extern inline bool axis_typed_##T##_list_node_init_empty(                     \
      axis_typed_##T##_list_node_t *self,                                       \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *));                                                  \
                                                                               \
  extern inline bool axis_typed_##T##_list_node_init(                           \
      axis_typed_##T##_list_node_t *self, T data,                               \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *));                                                  \
                                                                               \
  extern inline bool axis_typed_##T##_list_node_init_in_place(                  \
      axis_typed_##T##_list_node_t *self, void *data,                           \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *));                                                  \
                                                                               \
  extern inline axis_typed_##T##_list_node_t                                    \
      *axis_typed_##T##_list_node_create_empty(                                 \
          void (*construct)(T *, void *), /* NOLINT */                         \
          void (*move)(T *, T *),         /* NOLINT */                         \
          void (*copy)(T *, T *),         /* NOLINT */                         \
          void (*destruct)(T *));                                              \
                                                                               \
  extern inline axis_typed_##T##_list_node_t *axis_typed_##T##_list_node_create( \
      T data, void (*construct)(T *, void *), /* NOLINT */                     \
      void (*move)(T *, T *),                 /* NOLINT */                     \
      void (*copy)(T *, T *),                 /* NOLINT */                     \
      void (*destruct)(T *));                                                  \
                                                                               \
  extern inline axis_typed_##T##_list_node_t                                    \
      *axis_typed_##T##_list_node_create_in_place(                              \
          void *data, void (*construct)(T *, void *), /* NOLINT */             \
          void (*move)(T *, T *),                     /* NOLINT */             \
          void (*copy)(T *, T *),                     /* NOLINT */             \
          void (*destruct)(T *));                                              \
                                                                               \
  extern inline axis_typed_##T##_list_node_t *axis_typed_##T##_list_node_clone(  \
      axis_typed_##T##_list_node_t *src);                                       \
                                                                               \
  extern inline bool axis_typed_##T##_list_node_deinit(                         \
      axis_typed_##T##_list_node_t *self);                                      \
                                                                               \
  extern inline bool axis_typed_##T##_list_node_destroy(                        \
      axis_typed_##T##_list_node_t *self);                                      \
                                                                               \
  /* NOLINTNEXTLINE */                                                         \
  extern inline T *axis_typed_##T##_list_node_get_data(                         \
      axis_typed_##T##_list_node_t *self);                                      \
                                                                               \
  extern inline bool axis_typed_##T##_list_node_set_data(                       \
      axis_typed_##T##_list_node_t *self, T *data, bool move);

#define axis_typed_list_node_t(T) axis_typed_list_node_t_(T)
#define axis_typed_list_node_t_(T) axis_typed_##T##_list_node_t

#define axis_typed_list_node_init(T) axis_typed_list_node_init_(T)
#define axis_typed_list_node_init_(T) axis_typed_##T##_list_node_init

#define axis_typed_list_node_init_in_place(T) \
  axis_typed_list_node_init_in_place_(T)
#define axis_typed_list_node_init_in_place_(T) \
  axis_typed_##T##_list_node_init_in_place

#define axis_typed_list_node_create_empty(T) axis_typed_list_node_create_empty_(T)
#define axis_typed_list_node_create_empty_(T) \
  axis_typed_##T##_list_node_create_empty

#define axis_typed_list_node_create(T) axis_typed_list_node_create_(T)
#define axis_typed_list_node_create_(T) axis_typed_##T##_list_node_create

#define axis_typed_list_node_create_in_place(T) \
  axis_typed_list_node_create_in_place_(T)
#define axis_typed_list_node_create_in_place_(T) \
  axis_typed_##T##_list_node_create_in_place

#define axis_typed_list_node_clone(T) axis_typed_list_node_clone_(T)
#define axis_typed_list_node_clone_(T) axis_typed_##T##_list_node_clone

#define axis_typed_list_node_deinit(T) axis_typed_list_node_deinit_(T)
#define axis_typed_list_node_deinit_(T) axis_typed_##T##_list_node_deinit

#define axis_typed_list_node_destroy(T) axis_typed_list_node_destroy_(T)
#define axis_typed_list_node_destroy_(T) axis_typed_##T##_list_node_destroy

#define axis_typed_list_node_get_data(T) axis_typed_list_node_get_data_(T)
#define axis_typed_list_node_get_data_(T) axis_typed_##T##_list_node_get_data

#define axis_typed_list_node_set_data(T) axis_typed_list_node_set_data_(T)
#define axis_typed_list_node_set_data_(T) axis_typed_##T##_list_node_set_data
