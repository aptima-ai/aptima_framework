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

func (p *tenEnv) postSyncJob(payload job) any {
	// To prevent deadlock, we refuse to post jobs to the pool. So it's
	// recommended for developers to call async ten apis in goroutines other
	// than the ten goroutine.
	return payload()
}

func (p *tenEnv) postAsyncJob(payload job) any {
	return p.process(payload)
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
		C.bool(false),
	)

	return withGoStatus(&cStatus)
}

func (p *tenEnv) SendCmdEx(cmd Cmd, handler ResultHandler) error {
	if cmd == nil {
		return newTenError(
			ErrnoInvalidArgument,
			"cmd is required.",
		)
	}

	return withCGO(func() error {
		return p.sendCmdEx(cmd, handler)
	})
}

func (p *tenEnv) sendCmdEx(cmd Cmd, handler ResultHandler) error {
	defer cmd.keepAlive()

	cb := goHandleNil
	if handler != nil {
		cb = newGoHandle(handler)
	}

	cStatus := C.ten_go_ten_env_send_cmd(
		p.cPtr,
		cmd.getCPtr(),
		cHandle(cb),
		C.bool(true),
	)

	return withGoStatus(&cStatus)
}

// Exported function to be called from C when the async operation in C
// completes.
//
//export tenGoCAsyncApiCallback
func tenGoCAsyncApiCallback(
	callbackHandle C.uintptr_t,
	apiStatus C.ten_go_status_t,
) {
	// Start a Go routine for asynchronous processing to prevent blocking C code
	// on the native thread, which would in turn block the Go code calling the C
	// code.
	go func() {
		goHandle := goHandle(callbackHandle)
		done := loadAndDeleteGoHandle(goHandle).(chan error)

		err := withGoStatus(&apiStatus)

		done <- err
	}()
}

func (p *tenEnv) SendData(data Data) error {
	if data == nil {
		return newTenError(
			ErrnoInvalidArgument,
			"data is required.",
		)
	}

	defer data.keepAlive()

	// Create a channel to wait for the async operation in C to complete.
	done := make(chan error, 1)
	callbackHandle := newGoHandle(done)

	err := withCGO(func() error {
		apiStatus := C.ten_go_ten_env_send_data(
			p.cPtr,
			data.getCPtr(),
			C.uintptr_t(callbackHandle),
		)
		err := withGoStatus(&apiStatus)
		return err
	})

	if err != nil {
		// Clean up the handle if there was an error.
		loadAndDeleteGoHandle(callbackHandle)
		return err
	}

	// Wait for the async operation to complete.
	err = <-done

	return err
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
		C.ten_go_ten_env_on_create_instance_done(p.cPtr, instance.cPtr, C.uintptr_t(context))
	default:
		panic("instance must be extension or extension group.")
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
