//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package ten

// #include "extension_group.h"
import "C"

import (
	"fmt"
	"runtime"
	"unsafe"
)

// ExtensionGroup is the interface for the extension group.
type ExtensionGroup interface {
	OnInit(
		tenEnv TenEnv,
	)
	OnDeinit(tenEnv TenEnv)
	OnCreateExtensions(tenEnv TenEnv)
	OnDestroyExtensions(
		tenEnv TenEnv,
		extensions []Extension,
	)
}

// DefaultExtensionGroup is the default extension group.
type DefaultExtensionGroup struct{}

var _ ExtensionGroup = new(DefaultExtensionGroup)

// OnInit initializes the extension group.
func (p *DefaultExtensionGroup) OnInit(
	tenEnv TenEnv,
) {
	tenEnv.OnInitDone()
}

// OnDeinit deinitializes the extension group.
func (p *DefaultExtensionGroup) OnDeinit(
	tenEnv TenEnv,
) {
	tenEnv.OnDeinitDone()
}

// OnCreateExtensions creates the extensions.
func (p *DefaultExtensionGroup) OnCreateExtensions(
	tenEnv TenEnv,
) {
	extensions := make([]Extension, 0)
	tenEnv.OnCreateExtensionsDone(extensions...)
}

// OnDestroyExtensions deletes the extensions.
func (p *DefaultExtensionGroup) OnDestroyExtensions(
	tenEnv TenEnv,
	extensions []Extension,
) {
	tenEnv.OnDestroyExtensionsDone()
}

type extensionGroup struct {
	ExtensionGroup
	name string
	baseTenObject[C.uintptr_t]
}

// WrapExtensionGroup creates a new extension group.
func WrapExtensionGroup(
	extGroup ExtensionGroup,
	name string,
) *extensionGroup {
	extGroupInstance := &extensionGroup{
		ExtensionGroup: extGroup,
		name:           name,
	}

	extGroupObjID := newImmutableHandle(extGroupInstance)

	var bridge C.uintptr_t
	status := C.ten_go_extension_group_create(
		cHandle(extGroupObjID),
		unsafe.Pointer(unsafe.StringData(name)),
		C.int(len(name)),
		&bridge,
	)
	if err := withGoStatus(&status); err != nil {
		return nil
	}

	extGroupInstance.cPtr = bridge

	runtime.SetFinalizer(extGroupInstance, func(p *extensionGroup) {
		C.ten_go_extension_group_finalize(p.cPtr)
	})

	return extGroupInstance
}

func newExtensionGroupWithBridge(
	extGroup ExtensionGroup,
	name string,
	bridge C.uintptr_t,
) goHandle {
	extGroupInstance := &extensionGroup{
		ExtensionGroup: extGroup,
		name:           name,
	}

	extGroupInstance.cPtr = bridge

	runtime.SetFinalizer(extGroupInstance, func(p *extensionGroup) {
		C.ten_go_extension_group_finalize(p.cPtr)
	})

	return newImmutableHandle(extGroupInstance)
}

//
//export tenGoExtensionGroupOnInit
func tenGoExtensionGroupOnInit(
	extensionGroupID C.uintptr_t,
	tenEnvID C.uintptr_t,
) {
	extensionGroupObj, ok := loadImmutableHandle(goHandle(extensionGroupID)).(*extensionGroup)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get extension group from handle map, id: %d.",
				uintptr(extensionGroupID),
			),
		)
	}

	tenEnvObj, ok := handle(tenEnvID).get().(TenEnv)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten from handle map, id: %d.",
				uintptr(tenEnvID),
			),
		)
	}

	tenEnvInstance, ok := tenEnvObj.(*tenEnv)
	if !ok {
		// Should not happen.
		panic("Invalid ten object type.")
	}

	tenEnvInstance.attachToExtensionGroup(extensionGroupObj)

	extensionGroupObj.OnInit(
		tenEnvObj,
	)
}

//
//export tenGoExtensionGroupOnDeinit
func tenGoExtensionGroupOnDeinit(
	extensionGroupID C.uintptr_t,
	tenEnvID C.uintptr_t,
) {
	extensionGroupObj, ok := loadImmutableHandle(goHandle(extensionGroupID)).(*extensionGroup)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get extension group from handle map, id: %d.",
				uintptr(extensionGroupID),
			),
		)
	}

	tenEnvObj, ok := handle(tenEnvID).get().(TenEnv)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten from handle map, id: %d.",
				uintptr(tenEnvID),
			),
		)
	}

	extensionGroupObj.OnDeinit(tenEnvObj)
}

//
//export tenGoExtensionGroupOnCreateExtensions
func tenGoExtensionGroupOnCreateExtensions(
	extensionGroupID C.uintptr_t,
	tenEnvID C.uintptr_t,
) {
	extensionGroupObj, ok := loadImmutableHandle(goHandle(extensionGroupID)).(*extensionGroup)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get extension group from handle map, id: %d.",
				uintptr(extensionGroupID),
			),
		)
	}

	tenEnvObj, ok := handle(tenEnvID).get().(TenEnv)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten from handle map, id: %d.",
				uintptr(tenEnvID),
			),
		)
	}

	extensionGroupObj.OnCreateExtensions(tenEnvObj)
}

//
//export tenGoExtensionGroupOnDestroyExtensions
func tenGoExtensionGroupOnDestroyExtensions(
	extensionGroupID C.uintptr_t,
	tenEnvID C.uintptr_t,
	extensionArray *C.ten_go_handle_array_t,
) {
	extensionIds := convertCArrayToGoSlice(extensionArray)
	var extensions []Extension
	for _, id := range extensionIds {
		extensions = append(
			extensions,
			loadImmutableHandle(goHandle(id)).(*extension),
		)
	}

	extensionGroupObj, ok := loadImmutableHandle(goHandle(extensionGroupID)).(*extensionGroup)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get extension group from handle map, id: %d.",
				uintptr(extensionGroupID),
			),
		)
	}

	tenEnvObj, ok := handle(tenEnvID).get().(TenEnv)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten from handle map, id: %d.",
				uintptr(tenEnvID),
			),
		)
	}

	extensionGroupObj.OnDestroyExtensions(tenEnvObj, extensions)
}
