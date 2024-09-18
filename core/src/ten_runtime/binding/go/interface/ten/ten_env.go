//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package ten

//#include "ten_env.h"
import "C"

import (
	"fmt"
	"runtime"
	"strings"
	"unsafe"
)

type (
	// ResultHandler is a function type that represents a handler for the result
	// of a command.
	ResultHandler func(TenEnv, CmdResult)
)

// TenEnv represents the interface for the TEN (Run Time Environment) component.
type TenEnv interface {
	postSyncJob(payload job) any
	postAsyncJob(payload job) any

	setPropertyAsync(
		path string,
		v *value,
		callback func(TenEnv, error),
	)
	getPropertyAsync(path string, callback func(TenEnv, *value, error)) error

	SendJSON(json string, handler ResultHandler) error
	SendJSONBytes(json []byte, handler ResultHandler) error
	SendCmd(cmd Cmd, handler ResultHandler) error
	SendData(data Data) error
	SendVideoFrame(videoFrame VideoFrame) error
	SendAudioFrame(audioFrame AudioFrame) error

	ReturnResult(result CmdResult, cmd Cmd) error
	ReturnResultDirectly(result CmdResult) error

	OnConfigureDone() error
	OnInitDone() error
	OnStartDone() error
	OnStopDone() error
	OnDeinitDone() error

	OnCreateExtensionsDone(extensions ...Extension) error
	OnDestroyExtensionsDone() error

	OnCreateInstanceDone(instance any, context uintptr) error

	IsCmdConnected(cmdName string) (bool, error)

	AddonCreateExtensionAsync(
		addonName string,
		instanceName string,
		callback func(tenEnv TenEnv, p Extension),
	) error
	AddonDestroyExtensionAsync(
		ext Extension,
		callback func(tenEnv TenEnv),
	) error

	iProperty

	SetPropertyAsync(
		path string,
		v any,
		callback func(TenEnv, error),
	) error

	InitPropertyFromJSONBytes(value []byte) error

	LogVerbose(msg string)
	LogDebug(msg string)
	LogInfo(msg string)
	LogWarn(msg string)
	LogError(msg string)
	LogFatal(msg string)
	Log(level LogLevel, msg string)

	logInternal(level LogLevel, msg string, skip int)
}

// Making a compile-time assertion which indicates that if 'ten' type doesn't
// implement all the methods of the 'Ten' interface, the code will fail to
// compile, and you'll get an error. This is a way to ensure at compile time
// that a certain type satisfies an interface, even if you don't use that type
// to call the interface's methods in your code.
//
// In simpler terms, it's a way to say: "Make sure that the 'ten' type
// implements the 'Ten' interface. I don't need to use this check anywhere else
// in my code; I just want to make sure it's true." If 'ten' doesn't implement
// Ten, you'll know as soon as you try to compile.
var (
	_ TenEnv                            = new(tenEnv)
	_ iPropertyContainerForAsyncGeneric = new(tenEnv)
)

type tenAttachTo uint8

const (
	tenAttachToInvalid tenAttachTo = iota
	tenAttachToExtension
	tenAttachToExtensionGroup
	tenAttachToApp
	tenAttachToAddon
)

type tenEnv struct {
	baseTenObject[C.uintptr_t]

	attachToType tenAttachTo
	attachTo     unsafe.Pointer
}

func (p *tenEnv) attachToExtension(ext *extension) {
	if p.attachToType != tenAttachToInvalid {
		panic("The ten object can only be attached once.")
	}

	p.attachToType = tenAttachToExtension
	p.attachTo = unsafe.Pointer(ext)
}

func (p *tenEnv) attachToExtensionGroup(extGroup *extensionGroup) {
	if p.attachToType != tenAttachToInvalid {
		panic("The ten object can only be attached once.")
	}

	p.attachToType = tenAttachToExtensionGroup
	p.attachTo = unsafe.Pointer(extGroup)
}

func (p *tenEnv) postSyncJob(payload job) any {
	// To prevent deadlock, we refuse to post jobs to the pool. So it's
	// recommended for developers to call async ten apis in goroutines other
	// than the ten goroutine.
	return payload()
}

func (p *tenEnv) postAsyncJob(payload job) any {
	return p.process(payload)
}

func (p *tenEnv) sendJSON(
	bytes unsafe.Pointer,
	size int,
	handler ResultHandler,
) error {
	cb := goHandleNil
	if handler != nil {
		cb = newGoHandle(handler)
	}

	apiStatus := C.ten_go_ten_env_send_json(
		p.cPtr,
		bytes,
		C.int(size),
		cHandle(cb),
	)
	err := withGoStatus(&apiStatus)

	return err
}

// SendJSON sends a json string to TEN runtime, and the handler function will be
// called when the result (i.e., CmdResult) is received. The result will be
// discarded if the handler is nil.
func (p *tenEnv) SendJSON(json string, handler ResultHandler) error {
	if len(json) == 0 {
		return newTenError(
			ErrnoInvalidArgument,
			"json data is required.",
		)
	}

	return withCGO(func() error {
		return p.sendJSON(
			unsafe.Pointer(unsafe.StringData(json)),
			len(json),
			handler,
		)
	})
}

// SendJSONBytes sends a json bytes to TEN runtime, and the handler function
// will be called when the result (i.e., CmdResult) is received. The result
// will be discarded if the handler is nil.
func (p *tenEnv) SendJSONBytes(json []byte, handler ResultHandler) error {
	if len(json) == 0 {
		return newTenError(
			ErrnoInvalidArgument,
			"json data is required.",
		)
	}

	return withCGO(func() error {
		return p.sendJSON(
			unsafe.Pointer(unsafe.SliceData(json)),
			len(json),
			handler,
		)
	})
}

func (p *tenEnv) SendCmd(cmd Cmd, handler ResultHandler) error {
	if cmd == nil {
		return newTenError(
			ErrnoInvalidArgument,
			"cmd is required.",
		)
	}

	return withCGO(func() error {
		return p.sendCmd(cmd, handler)
	})
}

func (p *tenEnv) sendCmd(cmd Cmd, handler ResultHandler) error {
	defer cmd.keepAlive()

	cb := goHandleNil
	if handler != nil {
		cb = newGoHandle(handler)
	}

	cStatus := C.ten_go_ten_env_send_cmd(
		p.cPtr,
		cmd.getCPtr(),
		cHandle(cb),
	)

	return withGoStatus(&cStatus)
}

func (p *tenEnv) SendData(data Data) error {
	if data == nil {
		return newTenError(
			ErrnoInvalidArgument,
			"data is required.",
		)
	}

	defer data.keepAlive()

	return withCGO(func() error {
		apiStatus := C.ten_go_ten_env_send_data(p.cPtr, data.getCPtr())
		err := withGoStatus(&apiStatus)

		return err
	})
}

func (p *tenEnv) SendVideoFrame(videoFrame VideoFrame) error {
	if videoFrame == nil {
		return newTenError(
			ErrnoInvalidArgument,
			"videoFrame is required.",
		)
	}

	defer videoFrame.keepAlive()

	return withCGO(func() error {
		apiStatus := C.ten_go_ten_env_send_video_frame(
			p.cPtr,
			videoFrame.getCPtr(),
		)
		return withGoStatus(&apiStatus)
	})
}

func (p *tenEnv) SendAudioFrame(audioFrame AudioFrame) error {
	if audioFrame == nil {
		return newTenError(
			ErrnoInvalidArgument,
			"audioFrame is required.",
		)
	}

	res, ok := p.process(func() any {
		defer audioFrame.keepAlive()

		if res := C.ten_go_ten_env_send_audio_frame(p.cPtr, audioFrame.getCPtr()); !res {
			return newTenError(
				ErrnoGeneric,
				fmt.Sprintf("Failed to SendAudioFrame (%v)", audioFrame),
			)
		}
		return nil
	}).(error)
	if ok {
		return res
	}

	return nil
}

func (p *tenEnv) OnConfigureDone() error {
	p.LogDebug("OnConfigureDone")
	C.ten_go_ten_env_on_configure_done(p.cPtr)

	return nil
}

func (p *tenEnv) OnInitDone() error {
	p.LogDebug("OnInitDone")
	C.ten_go_ten_env_on_init_done(p.cPtr)

	return nil
}

func (p *tenEnv) OnStartDone() error {
	C.ten_go_ten_env_on_start_done(p.cPtr)

	return nil
}

func (p *tenEnv) OnStopDone() error {
	C.ten_go_ten_env_on_stop_done(p.cPtr)

	return nil
}

func (p *tenEnv) OnDeinitDone() error {
	C.ten_go_ten_env_on_deinit_done(p.cPtr)

	return nil
}

func (p *tenEnv) OnCreateExtensionsDone(extensions ...Extension) error {
	if len(extensions) == 0 {
		return nil
	}

	var extensionArray []C.uintptr_t
	for _, v := range extensions {
		extension, ok := v.(*extension)
		if !ok {
			panic("Invalid extension type")
		}

		extensionArray = append(extensionArray, extension.cPtr)
	}

	C.ten_go_ten_env_on_create_extensions_done(
		p.cPtr,
		unsafe.Pointer(unsafe.SliceData(extensionArray)),
		C.int(len(extensions)),
	)

	return nil
}

func (p *tenEnv) OnDestroyExtensionsDone() error {
	C.ten_go_ten_env_on_destroy_extensions_done(p.cPtr)

	return nil
}

func (p *tenEnv) OnCreateInstanceDone(instance any, context uintptr) error {
	switch instance := instance.(type) {
	case *extension:
		C.ten_go_ten_env_on_create_instance_done(p.cPtr, C.bool(true), instance.cPtr, C.uintptr_t(context))
	case *extensionGroup:
		C.ten_go_ten_env_on_create_instance_done(p.cPtr, C.bool(false), instance.cPtr, C.uintptr_t(context))
	default:
		panic("instance must be extension or extension group.")
	}

	return nil
}

func (p *tenEnv) setPropertyAsync(
	path string,
	v *value,
	callback func(TenEnv, error),
) {
	if v == nil || path == "" {
		callback(p, newTenError(
			ErrnoInvalidArgument,
			"path and value is required.",
		))
		return
	}

	defer v.keepAlive()

	callbackID := handle(0)
	if callback != nil {
		callbackID = newhandle(callback)
	}

	cName := C.CString(path)
	defer C.free(unsafe.Pointer(cName))

	if res := C.ten_go_ten_env_set_property_async(p.cPtr, cName, v.cPtr, C.uintptr_t(callbackID)); !res {
		callback(p, newTenError(
			ErrnoGeneric,
			"ten is closed.",
		))
	}
}

// Retrieve a property asynchronously from TEN world. It takes a property path
// and a callback function to be executed once the property is retrieved. The
// method interfaces with C code using cgo, and it ensures proper memory
// management and error handling.
func (p *tenEnv) getPropertyAsync(
	path string,
	callback func(TenEnv, *value, error),
) error {
	if callback == nil || path == "" {
		return newTenError(
			ErrnoInvalidArgument,
			"path and callback is required.",
		)
	}

	callbackID := newhandle(callback)

	cName := C.CString(path)
	// Ensures that the memory allocated for the C string is freed after the
	// function exits.
	defer C.free(unsafe.Pointer(cName))

	if res := C.ten_go_ten_env_get_property_async(p.cPtr, cName, C.uintptr_t(callbackID)); !res {
		return newTenError(
			ErrnoGeneric,
			"ten is closed.",
		)
	}
	return nil
}

func (p *tenEnv) IsCmdConnected(cmdName string) (bool, error) {
	return p.process(func() any {
		cName := C.CString(cmdName)
		defer C.free(unsafe.Pointer(cName))

		return bool(C.ten_go_ten_env_is_cmd_connected(p.cPtr, cName))
	}).(bool), nil
}

func (p *tenEnv) AddonCreateExtensionAsync(
	addonName string,
	instanceName string,
	callback func(tenEnv TenEnv, p Extension),
) error {
	handlerID := newhandle(callback)

	cAddonName := C.CString(addonName)
	defer C.free(unsafe.Pointer(cAddonName))

	cInstanceName := C.CString(instanceName)
	defer C.free(unsafe.Pointer(cInstanceName))

	res := bool(
		C.ten_go_ten_env_addon_create_extension(
			p.cPtr,
			cAddonName,
			cInstanceName,
			C.uintptr_t(handlerID),
		),
	)
	if !res {
		return newTenError(
			ErrnoGeneric,
			fmt.Sprintf("failed to find addon: %s", addonName),
		)
	}

	return nil
}

func (p *tenEnv) AddonDestroyExtensionAsync(
	ext Extension,
	callback func(tenEnv TenEnv),
) error {
	extension, ok := ext.(*extension)
	if !ok {
		return newTenError(
			ErrnoInvalidArgument,
			"wrong extension type.",
		)
	}

	handlerID := newhandle(callback)

	C.ten_go_ten_env_addon_destroy_extension(
		p.cPtr,
		extension.cPtr,
		C.uintptr_t(handlerID),
	)
	return nil
}

func (p *tenEnv) String() string {
	cString := C.ten_go_ten_env_debug_info(p.cPtr)
	defer C.free(unsafe.Pointer(cString))

	return C.GoString(cString)
}

func (p *tenEnv) SetPropertyAsync(
	path string,
	v any,
	callback func(TenEnv, error),
) error {
	if path == "" || v == nil {
		return newTenError(
			ErrnoInvalidArgument,
			"path and value is required.",
		)
	}

	res, ok := p.postAsyncJob(func() any {
		wrappedValue, err := wrapValueInner(v)
		if err != nil {
			return err
		}
		defer wrappedValue.free()

		p.setPropertyAsync(path, wrappedValue, callback)
		return nil
	}).(error)

	if ok {
		return res
	}
	return nil
}

func (p *tenEnv) LogVerbose(msg string) {
	p.logInternal(
		LogLevelVerbose, msg, 2)
}

func (p *tenEnv) LogDebug(msg string) {
	p.logInternal(LogLevelDebug, msg, 2)
}

func (p *tenEnv) LogInfo(msg string) {
	p.logInternal(LogLevelInfo, msg, 2)
}

func (p *tenEnv) LogWarn(msg string) {
	p.logInternal(LogLevelWarn, msg, 2)
}

func (p *tenEnv) LogError(msg string) {
	p.logInternal(LogLevelError, msg, 2)
}

func (p *tenEnv) LogFatal(msg string) {
	p.logInternal(LogLevelFatal, msg, 2)
}

func (p *tenEnv) Log(level LogLevel, msg string) {
	p.logInternal(level, msg, 1)
}

func (p *tenEnv) logInternal(level LogLevel, msg string, skip int) {
	// Get caller info.
	pc, fileName, lineNo, ok := runtime.Caller(skip)
	funcName := "unknown"
	if ok {
		fn := runtime.FuncForPC(pc)
		if fn != nil {
			funcName = fn.Name()

			parts := strings.Split(funcName, ".")
			if len(parts) > 0 {
				// The last part is the method name.
				funcName = parts[len(parts)-1]
			}
		}
	} else {
		fileName = "unknown"
		lineNo = 0
	}

	C.ten_go_ten_env_log(
		p.cPtr,
		C.int(level),
		unsafe.Pointer(unsafe.StringData(funcName)),
		C.int(len(funcName)),
		unsafe.Pointer(unsafe.StringData(fileName)),
		C.int(len(fileName)),
		C.int(lineNo),
		unsafe.Pointer(unsafe.StringData(msg)),
		C.int(len(msg)),
	)
}
