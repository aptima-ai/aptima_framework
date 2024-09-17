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
	"unsafe"
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

// NewDefaultExtensionAddon creates a new default extension addon.
func NewDefaultExtensionAddon(constructor func(name string) Extension) Addon {
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

// NewDefaultExtensionGroupAddon creates a new default extension group addon.
func NewDefaultExtensionGroupAddon(
	constructor func(name string) ExtensionGroup,
) Addon {
	return &DefaultAddon{
		onCreateInstanceImpl: func(tenEnv TenEnv, name string, context uintptr) {
			extGroup := constructor(name)
			if extGroup == nil {
				// Should not happen.
				panic("The extension group constructor returns nil.")
			}

			extGroupWrapper := WrapExtensionGroup(extGroup, name)
			tenEnv.OnCreateInstanceDone(extGroupWrapper, context)
		},
	}
}

// RegisterAddonAsExtension registers the addon as an extension.
func RegisterAddonAsExtension(addonName string, instance Addon) error {
	if len(addonName) == 0 {
		return newTenError(
			ErrnoInvalidArgument,
			"addon name is empty",
		)
	}

	addonWrapper := &addon{
		Addon: instance,
	}

	addonID := newImmutableHandle(addonWrapper)

	var bridge C.uintptr_t
	status := C.ten_go_addon_register_extension(
		unsafe.Pointer(unsafe.StringData(addonName)),
		C.int(len(addonName)),
		cHandle(addonID),
		&bridge,
	)

	if err := withGoStatus(&status); err != nil {
		loadAndDeleteImmutableHandle(addonID)
		return err
	}

	addonWrapper.cPtr = bridge

	return nil
}

// RegisterAddonAsExtensionGroup registers the addon as an extension group.
func RegisterAddonAsExtensionGroup(addonName string, instance Addon) error {
	if len(addonName) == 0 {
		return newTenError(
			ErrnoInvalidArgument,
			"addon name is empty",
		)
	}

	addonWrapper := &addon{
		Addon: instance,
	}

	addonID := newImmutableHandle(addonWrapper)

	var bridge C.uintptr_t
	status := C.ten_go_addon_register_extension_group(
		unsafe.Pointer(unsafe.StringData(addonName)),
		C.int(len(addonName)),
		cHandle(addonID),
		&bridge,
	)

	if err := withGoStatus(&status); err != nil {
		loadAndDeleteImmutableHandle(addonID)
		return err
	}

	addonWrapper.cPtr = bridge

	return nil
}

// unloadAllAddons unloads all addons.
func unloadAllAddons() error {
	clearImmutableHandles(func(value any) {
		if addon, ok := value.(*addon); ok {
			C.ten_go_addon_unregister(addon.cPtr)
		}
	})

	return nil
}

//
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
				"Failed to get ten from handle map, id: %d.",
				uintptr(tenEnvID),
			),
		)
	}

	addonObj.OnInit(tenEnvObj)
}

//
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
				"Failed to get ten from handle map, id: %d.",
				uintptr(tenEnvID),
			),
		)
	}

	addonObj.OnDeinit(tenEnvObj)
}

//
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
				"Failed to get ten from handle map, id: %d.",
				uintptr(tenEnvID),
			),
		)
	}

	extensionName := C.GoString(name)
	addonObj.OnCreateInstance(tenEnvObj, extensionName, uintptr(context))
}

//
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
