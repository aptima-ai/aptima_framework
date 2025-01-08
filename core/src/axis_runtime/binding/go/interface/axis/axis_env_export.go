//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package ten

//#include "axis_env.h"
import "C"

import (
	"fmt"
	"runtime"
)

//export tenGoCreateTenEnv
func tenGoCreateTenEnv(cInstance C.uintptr_t) C.uintptr_t {
	tenEnvInstance := &tenEnv{
		attachToType: tenAttachToInvalid,
	}
	tenEnvInstance.cPtr = cInstance
	tenEnvInstance.pool = newJobPool(5)
	runtime.SetFinalizer(tenEnvInstance, func(p *tenEnv) {
		C.axis_go_axis_env_finalize(p.cPtr)
	})

	id := newhandle(tenEnvInstance)
	tenEnvInstance.goObjID = id

	return C.uintptr_t(id)
}

//export tenGoDestroyTenEnv
func tenGoDestroyTenEnv(tenEnvObjID C.uintptr_t) {
	r, ok := handle(tenEnvObjID).free().(*tenEnv)

	r.attachToType = tenAttachToInvalid
	r.attachTo = nil

	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten env from handle map, id: %d.",
				uintptr(tenEnvObjID),
			),
		)
	} else {
		r.close()
	}
}

//export tenGoOnCmdResult
func tenGoOnCmdResult(
	tenEnvObjID C.uintptr_t,
	cmdResultBridge C.uintptr_t,
	resultHandler C.uintptr_t,
	cgoError C.axis_go_error_t,
) {
	tenEnvObj, ok := handle(tenEnvObjID).get().(TenEnv)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten env from handle map, id: %d.",
				uintptr(tenEnvObjID),
			),
		)
	}

	var cr *cmdResult = nil
	if cmdResultBridge == 0 {
		cr = nil
	} else {
		cr = newCmdResult(cmdResultBridge)
	}

	cb := loadAndDeleteGoHandle(goHandle(resultHandler))
	if cb == nil || cb == goHandleNil {
		// Should not happen.
		panic("The result handler is not found from handle map.")
	}

	err := withCGoError(&cgoError)

	cb.(ResultHandler)(tenEnvObj, cr, err)
}

//export tenGoOnError
func tenGoOnError(
	tenEnvObjID C.uintptr_t,
	errorHandler C.uintptr_t,
	cgoError C.axis_go_error_t,
) {
	tenEnvObj, ok := handle(tenEnvObjID).get().(TenEnv)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten env from handle map, id: %d.",
				uintptr(tenEnvObjID),
			),
		)
	}

	cb := loadAndDeleteGoHandle(goHandle(errorHandler))
	if cb == nil || cb == goHandleNil {
		// Should not happen.
		panic("The error handler is not found from handle map.")
	}

	err := withCGoError(&cgoError)

	cb.(ErrorHandler)(tenEnvObj, err)
}

//export tenGoSetPropertyCallback
func tenGoSetPropertyCallback(
	tenEnvObjID C.uintptr_t,
	handlerObjID C.uintptr_t,
	result C.bool,
) {
	tenEnvObj, ok := handle(tenEnvObjID).get().(TenEnv)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten env from handle map, id: %d.",
				uintptr(tenEnvObjID),
			),
		)
	}

	handlerObj, ok := handle(handlerObjID).free().(func(TenEnv, error))
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get callback from handle map, id: %d.",
				uintptr(handlerObjID),
			),
		)
	}

	if result {
		handlerObj(tenEnvObj, nil)
	} else {
		handlerObj(tenEnvObj, newTenError(
			ErrnoGeneric,
			"failed to set property",
		))
	}
}

//export tenGoGetPropertyCallback
func tenGoGetPropertyCallback(
	tenEnvObjID C.uintptr_t,
	handlerObjID C.uintptr_t,
	valueObjID C.uintptr_t,
) {
	tenEnvObj, ok := handle(tenEnvObjID).get().(TenEnv)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten env from handle map, id: %d.",
				uintptr(tenEnvObjID),
			),
		)
	}

	handlerObj, ok := handle(handlerObjID).free().(func(TenEnv, *value, error))
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get callback from handle map, id: %d.",
				uintptr(handlerObjID),
			),
		)
	}

	valueObj, ok := handle(valueObjID).get().(*value)
	if !ok {
		handlerObj(tenEnvObj, nil, newTenError(
			ErrnoGeneric,
			"failed to get property",
		))
		return
	}

	handlerObj(tenEnvObj, valueObj, nil)
}

//export tenGoOnAddonCreateExtensionDone
func tenGoOnAddonCreateExtensionDone(
	tenEnvObjID C.uintptr_t,
	addonObjID C.uintptr_t,
	extObjID C.uintptr_t,
	handlerObjID C.uintptr_t,
) {
	tenEnvObj, ok := handle(tenEnvObjID).get().(TenEnv)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten env from handle map, id: %d.",
				uintptr(tenEnvObjID),
			),
		)
	}

	extObj, ok := loadImmutableHandle(goHandle(extObjID)).(Extension)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get extension from handle map, id: %d.",
				uintptr(extObjID),
			),
		)
	}

	handlerObj, ok := handle(handlerObjID).free().(func(tenEnv TenEnv, p Extension))
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get callback from handle map, id: %d.",
				uintptr(handlerObjID),
			),
		)
	}

	handlerObj(tenEnvObj, extObj)
}

//export tenGoOnAddonDestroyExtensionDone
func tenGoOnAddonDestroyExtensionDone(
	tenEnvObjID C.uintptr_t,
	handlerObjID C.uintptr_t,
) {
	tenEnvObj, ok := handle(tenEnvObjID).get().(TenEnv)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten env from handle map, id: %d.",
				uintptr(tenEnvObjID),
			),
		)
	}

	handlerObj, ok := handle(handlerObjID).free().(func(tenEnv TenEnv))
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get callback from handle map, id: %d.",
				uintptr(handlerObjID),
			),
		)
	}

	handlerObj(tenEnvObj)
}
