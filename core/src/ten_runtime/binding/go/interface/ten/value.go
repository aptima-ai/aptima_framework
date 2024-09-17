//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package ten

//#include "value.h"
import "C"

import (
	"encoding/json"
	"runtime"
	"unsafe"
)

type value struct {
	baseTenObject[*C.ten_go_value_t]
}

// @{
// Functions: create ten value from go world.
func (p *value) int8() int8 {
	defer p.keepAlive()

	return int8(C.ten_go_value_get_int8_v1(p.cPtr))
}

func (p *value) int16() int16 {
	defer p.keepAlive()

	return int16(C.ten_go_value_get_int16_v1(p.cPtr))
}

func (p *value) int32() int32 {
	defer p.keepAlive()

	return int32(C.ten_go_value_get_int32_v1(p.cPtr))
}

func (p *value) int64() int64 {
	defer p.keepAlive()

	return int64(C.ten_go_value_get_int64_v1(p.cPtr))
}

func (p *value) uint8() uint8 {
	defer p.keepAlive()

	return uint8(C.ten_go_value_get_uint8_v1(p.cPtr))
}

func (p *value) uint16() uint16 {
	defer p.keepAlive()

	return uint16(C.ten_go_value_get_uint16_v1(p.cPtr))
}

func (p *value) uint32() uint32 {
	defer p.keepAlive()

	return uint32(C.ten_go_value_get_uint32_v1(p.cPtr))
}

func (p *value) uint64() uint64 {
	defer p.keepAlive()

	return uint64(C.ten_go_value_get_uint64_v1(p.cPtr))
}

func (p *value) float32() float32 {
	defer p.keepAlive()

	return float32(C.ten_go_value_get_float32_v1(p.cPtr))
}

func (p *value) float64() float64 {
	defer p.keepAlive()

	return float64(C.ten_go_value_get_float64_v1(p.cPtr))
}

func (p *value) getString() string {
	defer p.keepAlive()

	return C.GoString(C.ten_go_value_get_string_v1(p.cPtr))
}

func (p *value) bool() bool {
	defer p.keepAlive()

	return bool(C.ten_go_value_get_bool_v1(p.cPtr))
}

func (p *value) byteArray(clone bool) []byte {
	defer p.keepAlive()

	if clone {
		return C.GoBytes(
			C.ten_go_value_get_buf_data(p.cPtr),
			C.ten_go_value_get_buf_size(p.cPtr),
		)
	}
	return convertCByteArrayToGoSlice(
		C.ten_go_value_get_buf_data(p.cPtr),
		int(C.ten_go_value_get_buf_size(p.cPtr)),
	)
}

func (p *value) toJSON() string {
	defer p.keepAlive()

	cString := C.ten_go_value_to_json(p.cPtr)
	defer C.free(unsafe.Pointer(cString))

	return C.GoString(cString)
}

func getPtr[T any](p *value) (res T) {
	defer p.keepAlive()

	// T must be a pointer type.
	res = handle(C.ten_go_value_get_ptr_v1(p.cPtr)).get().(T)

	return
}

// @}

// @{
// Functions: create ten value from go world.
func tenValueCreateInt8(v int8) (*value, error) {
	id := C.ten_go_value_create_int8(C.int8_t(v))
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueCreateInt16(v int16) (*value, error) {
	id := C.ten_go_value_create_int16(C.int16_t(v))
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueCreateInt32(v int32) (*value, error) {
	id := C.ten_go_value_create_int32(C.int32_t(v))
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueCreateInt64(v int64) (*value, error) {
	id := C.ten_go_value_create_int64(C.int64_t(v))
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueCreateUint8(v uint8) (*value, error) {
	id := C.ten_go_value_create_uint8(C.uint8_t(v))
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueCreateUint16(v uint16) (*value, error) {
	id := C.ten_go_value_create_uint16(C.uint16_t(v))
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueCreateUint32(v uint32) (*value, error) {
	id := C.ten_go_value_create_uint32(C.uint32_t(v))
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueCreateUint64(v uint64) (*value, error) {
	id := C.ten_go_value_create_uint64(C.uint64_t(v))
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueCreateFloat32(v float32) (*value, error) {
	id := C.ten_go_value_create_float32(C.float(v))
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueCreateFloat64(v float64) (*value, error) {
	id := C.ten_go_value_create_float64(C.double(v))
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueCreateString(v string) (*value, error) {
	cString := C.CString(v)
	defer C.free(unsafe.Pointer(cString))

	id := C.ten_go_value_create_string(cString)
	res := handle(id).get().(*value)
	return res, nil
}

// TODO(Wei): Should the behavior of this function be changed to copy semantics?
func tenValueCreateByteArray(v []byte) (*value, error) {
	cData := C.CBytes(v)

	id := C.ten_go_value_create_byte_array(cData, C.int(len(v)))
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueCreateBool(v bool) (*value, error) {
	id := C.ten_go_value_create_bool(C.bool(v))
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueCreatePtr[T any](p T) (*value, error) {
	valueID := newhandle(p)

	id := C.ten_go_value_create_ptr(C.uintptr_t(valueID))
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueCreateFromJSON(jsonBytes []byte) (*value, error) {
	if !json.Valid(jsonBytes) {
		return nil, newTenError(
			ErrnoInvalidJSON,
			"invalid json.",
		)
	}
	jsonStr := string(jsonBytes)

	cString := C.CString(jsonStr)
	defer C.free(unsafe.Pointer(cString))

	id := C.ten_go_value_create_from_json(cString)
	res := handle(id).get().(*value)
	return res, nil
}

func tenValueUnwrap(
	cValue C.uintptr_t,
	pt propType,
	size uintptr,
) (any, error) {
	switch pt {
	case propTypeInt8:
		return getPropInt8(func(v *C.int8_t) C.ten_go_status_t {
			return C.ten_go_value_get_int8(cValue, v)
		})

	case propTypeInt16:
		return getPropInt16(func(v *C.int16_t) C.ten_go_status_t {
			return C.ten_go_value_get_int16(cValue, v)
		})

	case propTypeInt32:
		return getPropInt32(func(v *C.int32_t) C.ten_go_status_t {
			return C.ten_go_value_get_int32(cValue, v)
		})

	case propTypeInt64:
		return getPropInt64(func(v *C.int64_t) C.ten_go_status_t {
			return C.ten_go_value_get_int64(cValue, v)
		})

	case propTypeInt:
		if is64bit {
			return getPropInt64(func(v *C.int64_t) C.ten_go_status_t {
				return C.ten_go_value_get_int64(cValue, v)
			})
		}
		return getPropInt32(func(v *C.int32_t) C.ten_go_status_t {
			return C.ten_go_value_get_int32(cValue, v)
		})

	case propTypeUint8:
		return getPropUint8(func(v *C.uint8_t) C.ten_go_status_t {
			return C.ten_go_value_get_uint8(cValue, v)
		})

	case propTypeUint16:
		return getPropUint16(func(t *C.uint16_t) C.ten_go_status_t {
			return C.ten_go_value_get_uint16(cValue, t)
		})

	case propTypeUint32:
		return getPropUint32(func(v *C.uint32_t) C.ten_go_status_t {
			return C.ten_go_value_get_uint32(cValue, v)
		})

	case propTypeUint64:
		return getPropUint64(func(v *C.uint64_t) C.ten_go_status_t {
			return C.ten_go_value_get_uint64(cValue, v)
		})

	case propTypeUint:
		if is64bit {
			return getPropUint64(func(v *C.uint64_t) C.ten_go_status_t {
				return C.ten_go_value_get_uint64(cValue, v)
			})
		}
		return getPropUint32(func(v *C.uint32_t) C.ten_go_status_t {
			return C.ten_go_value_get_uint32(cValue, v)
		})

	case propTypeBool:
		return getPropBool(func(v *C.bool) C.ten_go_status_t {
			return C.ten_go_value_get_bool(cValue, v)
		})

	case propTypeFloat32:
		return getPropFloat32(func(v *C.float) C.ten_go_status_t {
			return C.ten_go_value_get_float32(cValue, v)
		})

	case propTypeFloat64:
		return getPropFloat64(func(v *C.double) C.ten_go_status_t {
			return C.ten_go_value_get_float64(cValue, v)
		})

	case propTypeStr:
		return getPropStr(size, func(v unsafe.Pointer) C.ten_go_status_t {
			return C.ten_go_value_get_string(cValue, v)
		})

	case propTypeBuf:
		return getPropBytes(size, func(v unsafe.Pointer) C.ten_go_status_t {
			return C.ten_go_value_get_buf(cValue, v)
		})

	case propTypePtr:
		return getPropPtr(func(v *cHandle) C.ten_go_status_t {
			return C.ten_go_value_get_ptr(cValue, v)
		})

	default:
		// Should not happen.
		return nil, newTenError(ErrnoInvalidType, "")
	}
}

func tenValueDestroy(cValue C.uintptr_t) {
	C.ten_go_value_destroy(cValue)
}

// @}

//export tenGoCreateValue
func tenGoCreateValue(cInstance *C.ten_go_value_t) C.uintptr_t {
	valueInstance := &value{}
	valueInstance.cPtr = cInstance
	runtime.SetFinalizer(valueInstance, func(p *value) {
		C.ten_go_value_finalize(p.cPtr)
	})

	id := newhandle(valueInstance)
	valueInstance.goObjID = id

	return C.uintptr_t(id)
}

//export tenGoUnrefObj
func tenGoUnrefObj(id C.uintptr_t) {
	handle(id).free()
}
