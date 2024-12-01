//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package ten

// #include "addon.h"
import "C"

import (
	"fmt"
)

// Addon is the interface for the addon.
type Addon interface {
	OnInit(tenEnv TenEnv)
	OnDeinit(tenEnv TenEnv)
	OnCreateInstance(tenEnv TenEnv, name string, context uintptr)
}

type DefaultAddon struct {
	onCreateInstanceImpl func(tenEnv TenEnv, name string, context uintptr)
}

func (p *DefaultAddon) OnInit(
	tenEnv TenEnv,
) {
	tenEnv.OnInitDone()
}

func (p *DefaultAddon) OnDeinit(tenEnv TenEnv) {
	tenEnv.OnDeinitDone()
}

func (p *DefaultAddon) OnCreateInstance(
	tenEnv TenEnv,
	name string,
	context uintptr,
) {
	if p.onCreateInstanceImpl == nil {
		panic("Not implemented")
	}

	p.onCreateInstanceImpl(tenEnv, name, context)
}

// TODO(Liu): move this definition to a internal package.
type addon struct {
	Addon

	baseTenObject[C.uintptr_t]
}

type ExtensionConstructor func(name string) Extension

// NewDefaultExtensionAddon creates a new default extension addon.
func NewDefaultExtensionAddon(constructor ExtensionConstructor) Addon {
	return &DefaultAddon{
		onCreateInstanceImpl: func(tenEnv TenEnv, name string, context uintptr) {
			ext := constructor(name)
			if ext == nil {
				// Should not happen.
				panic("The extension constructor returns nil.")
			}

			extWrapper := WrapExtension(ext, name)
			tenEnv.OnCreateInstanceDone(extWrapper, context)
		},
	}
}

//export tenGoAddonOnInit
func tenGoAddonOnInit(
	addonID C.uintptr_t,
	tenEnvID C.uintptr_t,
) {
	addonObj, ok := loadImmutableHandle(goHandle(addonID)).(*addon)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get addon from handle map, id: %d.",
				uintptr(addonID),
			),
		)
	}

	tenEnvObj, ok := handle(tenEnvID).get().(TenEnv)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten env from handle map, id: %d.",
				uintptr(tenEnvID),
			),
		)
	}

	addonObj.OnInit(tenEnvObj)
}

//export tenGoAddonOnDeinit
func tenGoAddonOnDeinit(addonID C.uintptr_t, tenEnvID C.uintptr_t) {
	addonObj, ok := loadImmutableHandle(goHandle(addonID)).(*addon)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get addon from handle map, id: %d.",
				uintptr(addonID),
			),
		)
	}

	tenEnvObj, ok := handle(tenEnvID).get().(TenEnv)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten env from handle map, id: %d.",
				uintptr(tenEnvID),
			),
		)
	}

	addonObj.OnDeinit(tenEnvObj)
}

//export tenGoAddonCreateInstance
func tenGoAddonCreateInstance(
	addonID C.uintptr_t,
	tenEnvID C.uintptr_t,
	name *C.char,
	context C.uintptr_t,
) {
	addonObj, ok := loadImmutableHandle(goHandle(addonID)).(*addon)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get addon from handle map, id: %d.",
				uintptr(addonID),
			),
		)
	}

	tenEnvObj, ok := handle(tenEnvID).get().(TenEnv)
	if !ok {
		panic(
			fmt.Sprintf(
				"Failed to get ten env from handle map, id: %d.",
				uintptr(tenEnvID),
			),
		)
	}

	extensionName := C.GoString(name)
	addonObj.OnCreateInstance(tenEnvObj, extensionName, uintptr(context))
}

//export tenGoAddonDestroyInstance
func tenGoAddonDestroyInstance(
	instanceID C.uintptr_t,
) {
	obj := loadAndDeleteImmutableHandle(goHandle(instanceID))
	if obj == nil {
		panic(
			fmt.Sprintf(
				"Failed to find addon instance from handle map, id: %d.",
				uintptr(instanceID),
			),
		)
	}
}
