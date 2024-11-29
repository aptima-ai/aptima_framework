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
	"path/filepath"
	"runtime"
	"sync"
	"unsafe"
)

// Define a registry map to store addon registration functions.
// The key is the addonName (string), and the value is a function that takes
// a registerCtx (interface{}) and returns an error.
var (
	addonRegistry      = make(map[string]func(interface{}) error)
	addonRegistryMutex = sync.RWMutex{}
)

// RegisterAddonAsExtension registers the addon as an extension.
func RegisterAddonAsExtension(addonName string, instance Addon) error {
	if len(addonName) == 0 {
		return newTenError(
			ErrnoInvalidArgument,
			"addon name is empty",
		)
	}

	_, file, _, ok := runtime.Caller(1)
	if !ok {
		return newTenError(ErrnoGeneric, "Failed to get the caller information")
	}

	baseDir := filepath.Dir(file)

	absBaseDir, err := filepath.Abs(baseDir)
	if err != nil {
		return newTenError(
			ErrnoGeneric,
			fmt.Sprintf("Failed to get the absolute file path: %v", err),
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
		unsafe.Pointer(unsafe.StringData(absBaseDir)),
		C.int(len(absBaseDir)),
		cHandle(addonID),
		&bridge,
	)

	if err := withCGoError(&status); err != nil {
		loadAndDeleteImmutableHandle(addonID)
		return err
	}

	addonWrapper.cPtr = bridge

	return nil
}

// RegisterAddonAsExtensionV2 registers the addon as an extension.
func RegisterAddonAsExtensionV2(addonName string, instance Addon) error {
	if len(addonName) == 0 {
		return newTenError(
			ErrnoInvalidArgument,
			"addon name is empty",
		)
	}

	_, file, _, ok := runtime.Caller(1)
	if !ok {
		return newTenError(ErrnoGeneric, "Failed to get the caller information")
	}

	baseDir := filepath.Dir(file)

	absBaseDir, err := filepath.Abs(baseDir)
	if err != nil {
		return newTenError(
			ErrnoGeneric,
			fmt.Sprintf("Failed to get the absolute file path: %v", err),
		)
	}

	addonWrapper := &addon{
		Addon: instance,
	}

	addonID := newImmutableHandle(addonWrapper)

	// Define the registration function that will be stored in the registry.
	registerFunc := func(registerCtx interface{}) error {
		var bridge C.uintptr_t
		status := C.ten_go_addon_register_extension_v2(
			unsafe.Pointer(unsafe.StringData(addonName)),
			C.int(len(addonName)),
			unsafe.Pointer(unsafe.StringData(absBaseDir)),
			C.int(len(absBaseDir)),
			cHandle(addonID),
			// TODO(Wei): Pass `register_ctx` to the actual cgo function.
			nil,
			&bridge,
		)

		if err := withCGoError(&status); err != nil {
			loadAndDeleteImmutableHandle(addonID)
			return err
		}

		addonWrapper.cPtr = bridge

		return nil
	}

	// Store the registration function in the registry map.
	addonRegistryMutex.Lock()
	defer addonRegistryMutex.Unlock()

	if _, exists := addonRegistry[addonName]; exists {
		return newTenError(
			ErrnoInvalidArgument,
			fmt.Sprintf("addon '%s' is already registered", addonName),
		)
	}

	addonRegistry[addonName] = registerFunc

	return nil
}

// LoadAllAddons executes all registered addon registration functions.
func LoadAllAddons(registerCtx interface{}) error {
	addonRegistryMutex.Lock()
	defer addonRegistryMutex.Unlock()

	for name, registerFunc := range addonRegistry {
		if err := registerFunc(registerCtx); err != nil {
			return fmt.Errorf("failed to register addon %s: %w", name, err)
		}
	}

	// Clear the addonRegistry to free up memory.
	addonRegistry = make(map[string]func(interface{}) error)

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
