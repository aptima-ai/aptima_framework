//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include "aptima_utils/macro/check.h"

#define aptima_TYPED_LIST_NODE_SIGNATURE 0x3CE1EAC77F72D345U

/**
 * @brief Because T is surrounded by ## in aptima_DEFINE_TYPED_LIST_NODE_(T), and
 * that would prevent T from expanding itself, so the following
 * aptima_DEFINE_TYPED_LIST_NODE() is to ensure T is expanded first before entering
 * into aptima_DEFINE_TYPED_LIST_NODE_(T)
 *
 * @param T The type of the value stored in the list node.
 */
#define aptima_DEFINE_TYPED_LIST_NODE(T) aptima_DEFINE_TYPED_LIST_NODE_(T)
#define aptima_DEFINE_TYPED_LIST_NODE_(T)                                         \
  typedef struct aptima_typed_##T##_list_node_t aptima_typed_##T##_list_node_t;      \
  struct aptima_typed_##T##_list_node_t {                                         \
    aptima_signature_t signature;                                                 \
    aptima_typed_##T##_list_node_t *next, *prev;                                  \
    T data;                                                                    \
    void (*construct)(T *, void *); /* NOLINT */                               \
    void (*move)(T *, T *);         /* NOLINT */                               \
    void (*copy)(T *, T *);         /* NOLINT */                               \
    void (*destruct)(T *);          /* NOLINT */                               \
  };                                                                           \
                                                                               \
  inline bool aptima_typed_##T##_list_node_check_integrity(                       \
      aptima_typed_##T##_list_node_t *self) {                                     \
    aptima_ASSERT(self, "Invalid argument.");                                     \
                                                                               \
    if (aptima_signature_get(&self->signature) !=                                 \
        aptima_TYPED_LIST_NODE_SIGNATURE) {                                       \
      aptima_ASSERT(0, "Should not happen.");                                     \
      return false;                                                            \
    }                                                                          \
    return true;                                                               \
  }                                                                            \
                                                                               \
  inline bool aptima_typed_##T##_list_node_init_empty(                            \
      aptima_typed_##T##_list_node_t *self, void (*construct)(T *, void *),       \
      void (*move)(T *, T *),  /* NOLINT */                                    \
      void (*copy)(T *, T *),  /* NOLINT */                                    \
      void (*destruct)(T *)) { /* NOLINT */                                    \
    aptima_ASSERT(self, "Invalid argument.");                                     \
                                                                               \
    aptima_signature_set(&self->signature, aptima_TYPED_LIST_NODE_SIGNATURE);        \
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
  inline bool aptima_typed_##T##_list_node_init(                                  \
      aptima_typed_##T##_list_node_t *self, T data,                               \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *)) {        /* NOLINT */                             \
    aptima_typed_##T##_list_node_init_empty(self, construct, move, copy,          \
                                         destruct);                            \
    self->data = data;                                                         \
    return true;                                                               \
  }                                                                            \
                                                                               \
  inline bool aptima_typed_##T##_list_node_init_in_place(                         \
      aptima_typed_##T##_list_node_t *self, void *data,                           \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *)) {        /* NOLINT */                             \
    aptima_typed_##T##_list_node_init_empty(self, construct, move, copy,          \
                                         destruct);                            \
    if (construct) {                                                           \
      construct(&self->data, data);                                            \
    }                                                                          \
    return true;                                                               \
  }                                                                            \
                                                                               \
  inline aptima_typed_##T##_list_node_t *aptima_typed_##T##_list_node_create_empty(  \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *)) {        /* NOLINT */                             \
    aptima_typed_##T##_list_node_t *self =                                        \
        (aptima_typed_##T##_list_node_t *)aptima_malloc(                             \
            sizeof(aptima_typed_##T##_list_node_t));                              \
    aptima_ASSERT(self, "Failed to allocate memory.");                            \
                                                                               \
    aptima_typed_##T##_list_node_init_empty(self, construct, move, copy,          \
                                         destruct);                            \
                                                                               \
    return self;                                                               \
  }                                                                            \
                                                                               \
  inline aptima_typed_##T##_list_node_t *aptima_typed_##T##_list_node_create(        \
      T data, void (*construct)(T *, void *), /* NOLINT */                     \
      void (*move)(T *, T *),                 /* NOLINT */                     \
      void (*copy)(T *, T *),                 /* NOLINT */                     \
      void (*destruct)(T *)) {                /* NOLINT */                     \
    aptima_typed_##T##_list_node_t *self =                                        \
        (aptima_typed_##T##_list_node_t *)aptima_malloc(                             \
            sizeof(aptima_typed_##T##_list_node_t));                              \
    aptima_ASSERT(self, "Failed to allocate memory.");                            \
                                                                               \
    aptima_typed_##T##_list_node_init(self, data, construct, move, copy,          \
                                   destruct);                                  \
    return self;                                                               \
  }                                                                            \
                                                                               \
  inline aptima_typed_##T##_list_node_t                                           \
      *aptima_typed_##T##_list_node_create_in_place(                              \
          void *data, void (*construct)(T *, void *), /* NOLINT */             \
          void (*move)(T *, T *),                     /* NOLINT */             \
          void (*copy)(T *, T *),                     /* NOLINT */             \
          void (*destruct)(T *)) {                    /* NOLINT */             \
    aptima_typed_##T##_list_node_t *self =                                        \
        (aptima_typed_##T##_list_node_t *)aptima_malloc(                             \
            sizeof(aptima_typed_##T##_list_node_t));                              \
    aptima_ASSERT(self, "Failed to allocate memory.");                            \
                                                                               \
    aptima_typed_##T##_list_node_init_in_place(self, data, construct, move, copy, \
                                            destruct);                         \
    return self;                                                               \
  }                                                                            \
                                                                               \
  inline aptima_typed_##T##_list_node_t *aptima_typed_##T##_list_node_clone(         \
      aptima_typed_##T##_list_node_t *src) {                                      \
    aptima_ASSERT(src, "Invalid argument.");                                      \
                                                                               \
    /* NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast) */               \
    aptima_typed_##T##_list_node_t *self =                                        \
        (aptima_typed_##T##_list_node_t *)aptima_malloc(                             \
            sizeof(aptima_typed_##T##_list_node_t));                              \
    aptima_ASSERT(self, "Failed to allocate memory.");                            \
                                                                               \
    aptima_typed_##T##_list_node_init_empty(self, src->construct, src->move,      \
                                         src->copy, src->destruct);            \
    src->copy(&self->data, &src->data);                                        \
    return self;                                                               \
  }                                                                            \
                                                                               \
  inline bool aptima_typed_##T##_list_node_deinit(                                \
      aptima_typed_##T##_list_node_t *self) {                                     \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_node_check_integrity(self),                \
               "Invalid argument.");                                           \
                                                                               \
    if (self->destruct) {                                                      \
      self->destruct(&(self->data));                                           \
    }                                                                          \
    return true;                                                               \
  }                                                                            \
                                                                               \
  inline bool aptima_typed_##T##_list_node_destroy(                               \
      aptima_typed_##T##_list_node_t *self) {                                     \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_node_check_integrity(self),                \
               "Invalid argument.");                                           \
                                                                               \
    if (!aptima_typed_##T##_list_node_deinit(self)) {                             \
      return false;                                                            \
    }                                                                          \
    aptima_free(self);                                                            \
    return true;                                                               \
  }                                                                            \
                                                                               \
  /* NOLINTNEXTLINE */                                                         \
  inline T *aptima_typed_##T##_list_node_get_data(                                \
      aptima_typed_##T##_list_node_t *self) {                                     \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_node_check_integrity(self),                \
               "Invalid argument.");                                           \
                                                                               \
    return &self->data;                                                        \
  }                                                                            \
                                                                               \
  inline bool aptima_typed_##T##_list_node_set_data(                              \
      aptima_typed_##T##_list_node_t *self, T *data, bool move) { /* NOLINT */    \
    aptima_ASSERT(self, "Invalid argument.");                                     \
    aptima_ASSERT(aptima_typed_##T##_list_node_check_integrity(self),                \
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

#define aptima_DECLARE_TYPED_LIST_NODE_INLINE_ASSETS(T) \
  aptima_DECLARE_TYPED_LIST_NODE_INLINE_ASSETS_(T)
#define aptima_DECLARE_TYPED_LIST_NODE_INLINE_ASSETS_(T)                          \
  extern inline bool aptima_typed_##T##_list_node_check_integrity(                \
      aptima_typed_##T##_list_node_t *self);                                      \
                                                                               \
  extern inline bool aptima_typed_##T##_list_node_init_empty(                     \
      aptima_typed_##T##_list_node_t *self,                                       \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *));                                                  \
                                                                               \
  extern inline bool aptima_typed_##T##_list_node_init(                           \
      aptima_typed_##T##_list_node_t *self, T data,                               \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *));                                                  \
                                                                               \
  extern inline bool aptima_typed_##T##_list_node_init_in_place(                  \
      aptima_typed_##T##_list_node_t *self, void *data,                           \
      void (*construct)(T *, void *), /* NOLINT */                             \
      void (*move)(T *, T *),         /* NOLINT */                             \
      void (*copy)(T *, T *),         /* NOLINT */                             \
      void (*destruct)(T *));                                                  \
                                                                               \
  extern inline aptima_typed_##T##_list_node_t                                    \
      *aptima_typed_##T##_list_node_create_empty(                                 \
          void (*construct)(T *, void *), /* NOLINT */                         \
          void (*move)(T *, T *),         /* NOLINT */                         \
          void (*copy)(T *, T *),         /* NOLINT */                         \
          void (*destruct)(T *));                                              \
                                                                               \
  extern inline aptima_typed_##T##_list_node_t *aptima_typed_##T##_list_node_create( \
      T data, void (*construct)(T *, void *), /* NOLINT */                     \
      void (*move)(T *, T *),                 /* NOLINT */                     \
      void (*copy)(T *, T *),                 /* NOLINT */                     \
      void (*destruct)(T *));                                                  \
                                                                               \
  extern inline aptima_typed_##T##_list_node_t                                    \
      *aptima_typed_##T##_list_node_create_in_place(                              \
          void *data, void (*construct)(T *, void *), /* NOLINT */             \
          void (*move)(T *, T *),                     /* NOLINT */             \
          void (*copy)(T *, T *),                     /* NOLINT */             \
          void (*destruct)(T *));                                              \
                                                                               \
  extern inline aptima_typed_##T##_list_node_t *aptima_typed_##T##_list_node_clone(  \
      aptima_typed_##T##_list_node_t *src);                                       \
                                                                               \
  extern inline bool aptima_typed_##T##_list_node_deinit(                         \
      aptima_typed_##T##_list_node_t *self);                                      \
                                                                               \
  extern inline bool aptima_typed_##T##_list_node_destroy(                        \
      aptima_typed_##T##_list_node_t *self);                                      \
                                                                               \
  /* NOLINTNEXTLINE */                                                         \
  extern inline T *aptima_typed_##T##_list_node_get_data(                         \
      aptima_typed_##T##_list_node_t *self);                                      \
                                                                               \
  extern inline bool aptima_typed_##T##_list_node_set_data(                       \
      aptima_typed_##T##_list_node_t *self, T *data, bool move);

#define aptima_typed_list_node_t(T) aptima_typed_list_node_t_(T)
#define aptima_typed_list_node_t_(T) aptima_typed_##T##_list_node_t

#define aptima_typed_list_node_init(T) aptima_typed_list_node_init_(T)
#define aptima_typed_list_node_init_(T) aptima_typed_##T##_list_node_init

#define aptima_typed_list_node_init_in_place(T) \
  aptima_typed_list_node_init_in_place_(T)
#define aptima_typed_list_node_init_in_place_(T) \
  aptima_typed_##T##_list_node_init_in_place

#define aptima_typed_list_node_create_empty(T) aptima_typed_list_node_create_empty_(T)
#define aptima_typed_list_node_create_empty_(T) \
  aptima_typed_##T##_list_node_create_empty

#define aptima_typed_list_node_create(T) aptima_typed_list_node_create_(T)
#define aptima_typed_list_node_create_(T) aptima_typed_##T##_list_node_create

#define aptima_typed_list_node_create_in_place(T) \
  aptima_typed_list_node_create_in_place_(T)
#define aptima_typed_list_node_create_in_place_(T) \
  aptima_typed_##T##_list_node_create_in_place

#define aptima_typed_list_node_clone(T) aptima_typed_list_node_clone_(T)
#define aptima_typed_list_node_clone_(T) aptima_typed_##T##_list_node_clone

#define aptima_typed_list_node_deinit(T) aptima_typed_list_node_deinit_(T)
#define aptima_typed_list_node_deinit_(T) aptima_typed_##T##_list_node_deinit

#define aptima_typed_list_node_destroy(T) aptima_typed_list_node_destroy_(T)
#define aptima_typed_list_node_destroy_(T) aptima_typed_##T##_list_node_destroy

#define aptima_typed_list_node_get_data(T) aptima_typed_list_node_get_data_(T)
#define aptima_typed_list_node_get_data_(T) aptima_typed_##T##_list_node_get_data

#define aptima_typed_list_node_set_data(T) aptima_typed_list_node_set_data_(T)
#define aptima_typed_list_node_set_data_(T) aptima_typed_##T##_list_node_set_data
