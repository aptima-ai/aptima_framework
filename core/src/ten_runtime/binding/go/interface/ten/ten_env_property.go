//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package ten

//#include "ten_env.h"
//#include "value.h"
import "C"

import (
	"fmt"
	"unsafe"
)

func (p *tenEnv) getPropertyTypeAndSize(
	path string,
	size *propSizeInC,
	cValue *C.uintptr_t,
) (propType, error) {
	defer p.keepAlive()

	var ptInC propTypeInC
	apiStatus := C.ten_go_ten_env_get_property_type_and_size(
		p.cPtr,
		unsafe.Pointer(unsafe.StringData(path)),
		C.int(len(path)),
		&ptInC,
		size,
		cValue,
	)
	err := withCGoError(&apiStatus)
	if err != nil {
		return propTypeInvalid, err
	}

	return propType(ptInC), nil
}

func (p *tenEnv) GetPropertyInt8(path string) (int8, error) {
	if len(path) == 0 {
		return 0, newTenError(
			ErrnoInvalidArgument,
			"property path is required",
		)
	}

	return getPropInt8(func(v *C.int8_t) C.ten_go_error_t {
		defer p.keepAlive()
		return C.ten_go_ten_env_get_property_int8(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			v,
		)
	})
}

func (p *tenEnv) GetPropertyInt16(path string) (int16, error) {
	if len(path) == 0 {
		return 0, newTenError(
			ErrnoInvalidArgument,
			"property path is required",
		)
	}

	return getPropInt16(func(v *C.int16_t) C.ten_go_error_t {
		defer p.keepAlive()
		return C.ten_go_ten_env_get_property_int16(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			v,
		)
	})
}

func (p *tenEnv) GetPropertyInt32(path string) (int32, error) {
	if len(path) == 0 {
		return 0, newTenError(
			ErrnoInvalidArgument,
			"property path is required",
		)
	}

	return getPropInt32(func(v *C.int32_t) C.ten_go_error_t {
		defer p.keepAlive()
		return C.ten_go_ten_env_get_property_int32(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			v,
		)
	})
}

func (p *tenEnv) GetPropertyInt64(path string) (int64, error) {
	if len(path) == 0 {
		return 0, newTenError(
			ErrnoInvalidArgument,
			"property path is required",
		)
	}

	return getPropInt64(func(v *C.int64_t) C.ten_go_error_t {
		defer p.keepAlive()
		return C.ten_go_ten_env_get_property_int64(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			v,
		)
	})
}

func (p *tenEnv) GetPropertyUint8(path string) (uint8, error) {
	if len(path) == 0 {
		return 0, newTenError(
			ErrnoInvalidArgument,
			"property path is required",
		)
	}

	return getPropUint8(func(v *C.uint8_t) C.ten_go_error_t {
		defer p.keepAlive()
		return C.ten_go_ten_env_get_property_uint8(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			v,
		)
	})
}

func (p *tenEnv) GetPropertyUint16(path string) (uint16, error) {
	if len(path) == 0 {
		return 0, newTenError(
			ErrnoInvalidArgument,
			"property path is required",
		)
	}

	return getPropUint16(func(v *C.uint16_t) C.ten_go_error_t {
		defer p.keepAlive()
		return C.ten_go_ten_env_get_property_uint16(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			v,
		)
	})
}

func (p *tenEnv) GetPropertyUint32(path string) (uint32, error) {
	if len(path) == 0 {
		return 0, newTenError(
			ErrnoInvalidArgument,
			"property path is required",
		)
	}

	return getPropUint32(func(v *C.uint32_t) C.ten_go_error_t {
		defer p.keepAlive()
		return C.ten_go_ten_env_get_property_uint32(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			v,
		)
	})
}

func (p *tenEnv) GetPropertyUint64(path string) (uint64, error) {
	if len(path) == 0 {
		return 0, newTenError(
			ErrnoInvalidArgument,
			"property path is required",
		)
	}

	return getPropUint64(func(v *C.uint64_t) C.ten_go_error_t {
		defer p.keepAlive()
		return C.ten_go_ten_env_get_property_uint64(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			v,
		)
	})
}

func (p *tenEnv) GetPropertyFloat32(path string) (float32, error) {
	if len(path) == 0 {
		return 0, newTenError(
			ErrnoInvalidArgument,
			"property path is required",
		)
	}

	return getPropFloat32(func(v *C.float) C.ten_go_error_t {
		defer p.keepAlive()
		return C.ten_go_ten_env_get_property_float32(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			v,
		)
	})
}

func (p *tenEnv) GetPropertyFloat64(path string) (float64, error) {
	if len(path) == 0 {
		return 0, newTenError(
			ErrnoInvalidArgument,
			"property path is required",
		)
	}

	return getPropFloat64(func(v *C.double) C.ten_go_error_t {
		defer p.keepAlive()
		return C.ten_go_ten_env_get_property_float64(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			v,
		)
	})
}

func (p *tenEnv) GetPropertyBool(path string) (bool, error) {
	if len(path) == 0 {
		return false, newTenError(
			ErrnoInvalidArgument,
			"property path is required",
		)
	}

	return getPropBool(func(v *C.bool) C.ten_go_error_t {
		defer p.keepAlive()
		return C.ten_go_ten_env_get_property_bool(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			v,
		)
	})
}

func (p *tenEnv) GetPropertyPtr(path string) (any, error) {
	if len(path) == 0 {
		return nil, newTenError(
			ErrnoInvalidArgument,
			"property path is required",
		)
	}

	return getPropPtr(func(v *cHandle) C.ten_go_error_t {
		defer p.keepAlive()
		return C.ten_go_ten_env_get_property_ptr(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			v,
		)
	})
}

// GetPropertyString retrieves the string property stored in a ten. This
// function has one less memory allocation than calling GetProperty.
//
// Any type conversions to or from the `any` type may lead the Go compiler to
// generate functions like runtime.convT64, runtime.convTslice, or similar
// functions to perform the conversions. These runtime.convTxxx functions might
// lead to memory allocations. The return type of GetProperty is any, so this
// function with a return type that is not any is created to avoid the memory
// allocations associated with type conversions involving the `any` type.
func (p *tenEnv) GetPropertyString(path string) (string, error) {
	if len(path) == 0 {
		return "", newTenError(
			ErrnoInvalidArgument,
			"the property path is required",
		)
	}

	var cValue C.uintptr_t = 0
	var pSize propSizeInC = 0
	realPt, err := p.getPropertyTypeAndSize(path, &pSize, &cValue)
	if err != nil {
		return "", err
	}

	// The value should be found if no error.
	if cValue == 0 {
		return "", newTenError(
			ErrnoGeneric,
			"Should not happen.",
		)
	}

	if realPt != propTypeStr {
		// The ten_value_t is cloned in TEN runtime, so we have to destroy it.
		tenValueDestroy(cValue)
		return "", newTenError(
			ErrnoInvalidType,
			fmt.Sprintf("expected: %s, actual: %s", propTypeStr, realPt),
		)
	}

	if pSize == 0 {
		tenValueDestroy(cValue)

		// We can not allocate a []byte with size 0, so just return "".
		return "", nil
	}

	return getPropStr(
		uintptr(pSize),
		func(v unsafe.Pointer) C.ten_go_error_t {
			defer p.keepAlive()
			return C.ten_go_value_get_string(cValue, v)
		},
	)
}

// GetPropertyBytes retrieves the []byte property stored in a ten. This function
// has one less memory allocation than calling GetProperty.
//
// Any type conversions to or from the `any` type may lead the Go compiler to
// generate functions like runtime.convT64, runtime.convTslice, or similar
// functions to perform the conversions. These runtime.convTxxx functions might
// lead to memory allocations. The return type of GetProperty is any, so this
// function with a return type that is not any is created to avoid the memory
// allocations associated with type conversions involving the `any` type.
func (p *tenEnv) GetPropertyBytes(path string) ([]byte, error) {
	if len(path) == 0 {
		return nil, newTenError(
			ErrnoInvalidArgument,
			"the property path is required",
		)
	}

	var cValue C.uintptr_t = 0
	var pSize propSizeInC = 0
	realPt, err := p.getPropertyTypeAndSize(path, &pSize, &cValue)
	if err != nil {
		return nil, err
	}

	// The value should be found if no error.
	if cValue == 0 {
		return nil, newTenError(
			ErrnoGeneric,
			"Should not happen.",
		)
	}

	if realPt != propTypeBuf {
		// The ten_value_t is cloned in TEN runtime, so we have to destroy it.
		tenValueDestroy(cValue)
		return nil, newTenError(
			ErrnoInvalidType,
			fmt.Sprintf("expected: %s, actual: %s", propTypeBuf, realPt),
		)
	}

	if pSize == 0 {
		tenValueDestroy(cValue)

		// We can not allocate a []byte with size 0, so just return nil.
		return nil, nil
	}

	return getPropBytes(
		uintptr(pSize),
		func(v unsafe.Pointer) C.ten_go_error_t {
			defer p.keepAlive()
			return C.ten_go_value_get_buf(cValue, v)
		},
	)
}

// SetProperty sets the value as a property in the ten. Note that there will be
// a type conversion which causes memory allocation if the type of value is
// string or []byte. If the performance is critical, calling the concrete method
// SetPropertyString or SetPropertyBytes is more appropriate.
func (p *tenEnv) SetProperty(path string, value any) error {
	if len(path) == 0 {
		return newTenError(
			ErrnoInvalidArgument,
			"the property path is required",
		)
	}

	pt := getPropType(value)
	if err := pt.isTypeSupported(); err != nil {
		return err
	}

	return withCGO(func() error {
		// Create a channel to wait for the async operation in C to complete.
		done := make(chan error, 1)
		callbackHandle := newGoHandle(done)

		var err error
		switch pt {
		case propTypeBool:
			apiStatus := C.ten_go_ten_env_set_property_bool(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				C.bool(value.(bool)),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		case propTypeInt8:
			apiStatus := C.ten_go_ten_env_set_property_int8(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				C.int8_t(value.(int8)),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		case propTypeInt16:
			apiStatus := C.ten_go_ten_env_set_property_int16(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				C.int16_t(value.(int16)),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		case propTypeInt32:
			apiStatus := C.ten_go_ten_env_set_property_int32(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				C.int32_t(value.(int32)),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		case propTypeInt64:
			apiStatus := C.ten_go_ten_env_set_property_int64(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				C.int64_t(value.(int64)),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		case propTypeInt:
			if is64bit {
				apiStatus := C.ten_go_ten_env_set_property_int64(
					p.cPtr,
					unsafe.Pointer(unsafe.StringData(path)),
					C.int(len(path)),
					C.int64_t(value.(int)),
					C.uintptr_t(callbackHandle),
				)
				err = withCGoError(&apiStatus)
			} else {
				apiStatus := C.ten_go_ten_env_set_property_int32(
					p.cPtr,
					unsafe.Pointer(unsafe.StringData(path)),
					C.int(len(path)),
					C.int32_t(value.(int)),
					C.uintptr_t(callbackHandle),
				)
				err = withCGoError(&apiStatus)
			}

		case propTypeUint8:
			apiStatus := C.ten_go_ten_env_set_property_uint8(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				C.uint8_t(value.(uint8)),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		case propTypeUint16:
			apiStatus := C.ten_go_ten_env_set_property_uint16(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				C.uint16_t(value.(uint16)),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		case propTypeUint32:
			apiStatus := C.ten_go_ten_env_set_property_uint32(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				C.uint32_t(value.(uint32)),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		case propTypeUint64:
			apiStatus := C.ten_go_ten_env_set_property_uint64(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				C.uint64_t(value.(uint64)),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		case propTypeUint:
			if is64bit {
				apiStatus := C.ten_go_ten_env_set_property_uint64(
					p.cPtr,
					unsafe.Pointer(unsafe.StringData(path)),
					C.int(len(path)),
					C.uint64_t(value.(uint)),
					C.uintptr_t(callbackHandle),
				)
				err = withCGoError(&apiStatus)
			} else {
				apiStatus := C.ten_go_ten_env_set_property_uint32(
					p.cPtr,
					unsafe.Pointer(unsafe.StringData(path)),
					C.int(len(path)),
					C.uint32_t(value.(uint)),
					C.uintptr_t(callbackHandle),
				)
				err = withCGoError(&apiStatus)
			}

		case propTypeFloat32:
			apiStatus := C.ten_go_ten_env_set_property_float32(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				C.float(value.(float32)),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		case propTypeFloat64:
			apiStatus := C.ten_go_ten_env_set_property_float64(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				C.double(value.(float64)),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		case propTypeStr:
			vs := value.(string)
			apiStatus := C.ten_go_ten_env_set_property_string(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				unsafe.Pointer(unsafe.StringData(vs)),
				C.int(len(vs)),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		case propTypeBuf:
			vb := value.([]byte)
			apiStatus := C.ten_go_ten_env_set_property_buf(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				unsafe.Pointer(unsafe.SliceData(vb)),
				C.int(len(vb)),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		case propTypePtr:
			vh := newGoHandle(value)
			apiStatus := C.ten_go_ten_env_set_property_ptr(
				p.cPtr,
				unsafe.Pointer(unsafe.StringData(path)),
				C.int(len(path)),
				cHandle(vh),
				C.uintptr_t(callbackHandle),
			)
			err = withCGoError(&apiStatus)

		default:
			// Should not happen.
			err = newTenError(ErrnoInvalidType, "")
		}

		if err != nil {
			// Clean up the handle if there was an error.
			loadAndDeleteGoHandle(callbackHandle)
			return err
		}

		// Wait for the async operation to complete.
		err = <-done

		return err
	})
}

// SetPropertyString sets a string as property in the ten. This function has one
// less memory allocation than calling SetProperty.
func (p *tenEnv) SetPropertyString(
	path string,
	value string,
) error {
	if len(path) == 0 {
		return newTenError(
			ErrnoInvalidArgument,
			"the property path is required",
		)
	}

	return withCGO(func() error {
		// Create a channel to wait for the async operation in C to complete.
		done := make(chan error, 1)
		callbackHandle := newGoHandle(done)

		apiStatus := C.ten_go_ten_env_set_property_string(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			unsafe.Pointer(unsafe.StringData(value)),
			C.int(len(value)),
			C.uintptr_t(callbackHandle),
		)
		err := withCGoError(&apiStatus)
		if err != nil {
			// Clean up the handle if there was an error.
			loadAndDeleteGoHandle(callbackHandle)
			return err
		}

		// Wait for the async operation to complete.
		err = <-done

		return err
	})
}

// SetPropertyBytes sets a []byte as property in the ten. This function has one
// less memory allocation than calling SetProperty.
func (p *tenEnv) SetPropertyBytes(
	path string,
	value []byte,
) error {
	if len(path) == 0 || len(value) == 0 {
		return newTenError(
			ErrnoInvalidArgument,
			"the property value is required",
		)
	}

	return withCGO(func() error {
		// Create a channel to wait for the async operation in C to complete.
		done := make(chan error, 1)
		callbackHandle := newGoHandle(done)

		apiStatus := C.ten_go_ten_env_set_property_buf(
			p.cPtr,
			unsafe.Pointer(unsafe.StringData(path)),
			C.int(len(path)),
			// Using either `unsafe.SliceData()` or `&value[0]` works fine. And
			// `unsafe.SliceData()` reduces one memory allocation, while
			// `runtime.convTslice` will be called if using `&value[0]`.
			unsafe.Pointer(unsafe.SliceData(value)),
			C.int(len(value)),
			C.uintptr_t(callbackHandle),
		)
		err := withCGoError(&apiStatus)
		if err != nil {
			// Clean up the handle if there was an error.
			loadAndDeleteGoHandle(callbackHandle)
			return err
		}

		// Wait for the async operation to complete.
		err = <-done

		return err
	})
}

func (p *tenEnv) setPropertyFromJSONBytes(path string, value []byte) error {
	defer p.keepAlive()

	// Create a channel to wait for the async operation in C to complete.
	done := make(chan error, 1)
	callbackHandle := newGoHandle(done)

	apiStatus := C.ten_go_ten_env_set_property_json_bytes(
		p.cPtr,
		unsafe.Pointer(unsafe.StringData(path)),
		C.int(len(path)),
		unsafe.Pointer(unsafe.SliceData(value)),
		C.int(len(value)),
		C.uintptr_t(callbackHandle),
	)
	err := withCGoError(&apiStatus)
	if err != nil {
		// Clean up the handle if there was an error.
		loadAndDeleteGoHandle(callbackHandle)
		return err
	}

	// Wait for the async operation to complete.
	err = <-done

	return err
}

// SetPropertyFromJSONBytes sets a json data as a property in the ten. The
// `value` must be a valid json data. The json data will be treated as an object
// or array in TEN runtime, but not a slice. The usual practice is to use
// GetPropertyToJSONBytes to extract everything at once. However, if the
// structure is already known beforehand through certain methods, GetProperty
// can be used to retrieve individual fields.
func (p *tenEnv) SetPropertyFromJSONBytes(path string, value []byte) error {
	if len(path) == 0 || len(value) == 0 {
		return newTenError(
			ErrnoInvalidArgument,
			"the property path and value are required",
		)
	}

	return withCGO(func() error {
		return p.setPropertyFromJSONBytes(path, value)
	})
}

func (p *tenEnv) getPropertyToJSONBytes(path string) ([]byte, error) {
	defer p.keepAlive()

	var cJSONStr *C.char
	var cJSONStrLen C.uintptr_t
	apiStatus := C.ten_go_ten_env_get_property_json_and_size(
		p.cPtr,
		unsafe.Pointer(unsafe.StringData(path)),
		C.int(len(path)),
		&cJSONStrLen,
		&cJSONStr,
	)
	err := withCGoError(&apiStatus)
	if err != nil {
		return nil, err
	}

	size := uintptr(cJSONStrLen)
	if size == 0 {
		// The size of any valid json could not be 0.
		return nil, newTenError(
			ErrnoInvalidJSON,
			"the size of json data is 0",
		)
	}

	// Refer to the comments in `getPropertyToJSONBytes` in msg.go for the
	// reason why we do not use `C.GoBytes`.
	return getPropBytes(size, func(v unsafe.Pointer) C.ten_go_error_t {
		return C.ten_go_copy_c_str_to_slice_and_free(cJSONStr, v)
	})
}

// GetPropertyToJSONBytes retrieves the property and parses it as a json data.
// This function uses a bytes pool to improve the performance. ReleaseBytes is
// recommended to be called after the []byte is no longer used.
func (p *tenEnv) GetPropertyToJSONBytes(path string) ([]byte, error) {
	if len(path) == 0 {
		return nil, newTenError(
			ErrnoInvalidArgument,
			"the property path is required.",
		)
	}

	return withCGOLimiterHasReturnValue[[]byte](func() ([]byte, error) {
		return p.getPropertyToJSONBytes(path)
	})
}

func (p *tenEnv) InitPropertyFromJSONBytes(value []byte) error {
	defer p.keepAlive()

	apiStatus := C.ten_go_ten_env_init_property_from_json_bytes(
		p.cPtr,
		unsafe.Pointer(unsafe.SliceData(value)),
		C.int(len(value)),
	)
	err := withCGoError(&apiStatus)

	return err
}
