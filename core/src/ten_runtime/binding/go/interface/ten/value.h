//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

typedef struct ten_value_t ten_value_t;
typedef struct ten_go_value_t ten_go_value_t;

// @{
// Functions: create ten value from go world.
ten_go_handle_t ten_go_value_create_int8(int8_t v);

ten_go_handle_t ten_go_value_create_int16(int16_t v);

ten_go_handle_t ten_go_value_create_int32(int32_t v);

ten_go_handle_t ten_go_value_create_int64(int64_t v);

ten_go_handle_t ten_go_value_create_uint8(uint8_t v);

ten_go_handle_t ten_go_value_create_uint16(uint16_t v);

ten_go_handle_t ten_go_value_create_uint32(uint32_t v);

ten_go_handle_t ten_go_value_create_uint64(uint64_t v);

ten_go_handle_t ten_go_value_create_float32(float v);

ten_go_handle_t ten_go_value_create_float64(double v);

ten_go_handle_t ten_go_value_create_string(const char *v);

ten_go_handle_t ten_go_value_create_bool(bool v);

ten_go_handle_t ten_go_value_create_byte_array(void *buf, int len);

ten_go_handle_t ten_go_value_create_ptr(ten_go_handle_t v);

ten_go_handle_t ten_go_value_create_from_json(const char *json_str);
// @}

// @{
// Functions: get ten value from go world.
int8_t ten_go_value_get_int8_v1(ten_go_value_t *self);

int16_t ten_go_value_get_int16_v1(ten_go_value_t *self);

int32_t ten_go_value_get_int32_v1(ten_go_value_t *self);

int64_t ten_go_value_get_int64_v1(ten_go_value_t *self);

uint8_t ten_go_value_get_uint8_v1(ten_go_value_t *self);

uint16_t ten_go_value_get_uint16_v1(ten_go_value_t *self);

uint32_t ten_go_value_get_uint32_v1(ten_go_value_t *self);

uint64_t ten_go_value_get_uint64_v1(ten_go_value_t *self);

float ten_go_value_get_float32_v1(ten_go_value_t *self);

double ten_go_value_get_float64_v1(ten_go_value_t *self);

const char *ten_go_value_get_string_v1(ten_go_value_t *self);

bool ten_go_value_get_bool_v1(ten_go_value_t *self);

void *ten_go_value_get_buf_data(ten_go_value_t *self);

int ten_go_value_get_buf_size(ten_go_value_t *self);

ten_go_handle_t ten_go_value_get_ptr_v1(ten_go_value_t *self);

const char *ten_go_value_to_json(ten_go_value_t *self);
// @}

void ten_go_value_finalize(ten_go_value_t *self);

// {@

// These functions are used in getting property from ten_env_t. Refer to the
// comments in ten.h. Please keep in mind that the input ten_vale_t* is cloned
// in the previous stage (refer to ten_go_ten_property_get_type_and_size), so it
// must be destroyed in these functions.

/**
 * @param value_addr The bit pattern of the pointer to a ten_value_t. Note that
 * there is no bridge for ten_value_t.
 */
ten_go_status_t ten_go_value_get_int8(uintptr_t value_addr, int8_t *value);

ten_go_status_t ten_go_value_get_int16(uintptr_t value_addr, int16_t *value);

ten_go_status_t ten_go_value_get_int32(uintptr_t value_addr, int32_t *value);

ten_go_status_t ten_go_value_get_int64(uintptr_t value_addr, int64_t *value);

ten_go_status_t ten_go_value_get_uint8(uintptr_t value_addr, uint8_t *value);

ten_go_status_t ten_go_value_get_uint16(uintptr_t value_addr, uint16_t *value);

ten_go_status_t ten_go_value_get_uint32(uintptr_t value_addr, uint32_t *value);

ten_go_status_t ten_go_value_get_uint64(uintptr_t value_addr, uint64_t *value);

ten_go_status_t ten_go_value_get_float32(uintptr_t value_addr, float *value);

ten_go_status_t ten_go_value_get_float64(uintptr_t value_addr, double *value);

ten_go_status_t ten_go_value_get_bool(uintptr_t value_addr, bool *value);

ten_go_status_t ten_go_value_get_string(uintptr_t value_addr, void *value);

ten_go_status_t ten_go_value_get_buf(uintptr_t value_addr, void *value);

ten_go_status_t ten_go_value_get_ptr(uintptr_t value_addr,
                                     ten_go_handle_t *value);

// @}

/**
 * @brief Destroy the ten_value_t instance from GO.
 *
 * @param value_addr The bit pattern of the pointer to a ten_value_t. Note that
 * there is no bridge for ten_value_t.
 */
void ten_go_value_destroy(uintptr_t value_addr);
