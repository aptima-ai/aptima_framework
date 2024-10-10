// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/TEN-framework/ten_framework/LICENSE for more
// information.
//
// Note that this is just an example extension written in the GO programming
// language, so the package name does not equal to the containing directory
// name. However, it is not common in Go.
package default_extension_go

import (
	"fmt"
	"time"

	"ten_framework/ten"
)

type addonImpl struct {
	ten.DefaultAddon
}

func (p *addonImpl) OnCreateInstance(
	tenEnv ten.TenEnv,
	name string,
	context uintptr,
) {
	switch name {
	case "A":
		ext := &aExtension{name: name, isStopped: false}
		tenEnv.OnCreateInstanceDone(ten.WrapExtension(ext, "A"), context)
	case "B":
		ext := NewBExtension()
		tenEnv.OnCreateInstanceDone(ten.WrapExtension(ext, "B"), context)
	case "C":
		ext := NewCExtension()
		tenEnv.OnCreateInstanceDone(ten.WrapExtension(ext, "C"), context)
	default:
		panic("Should not happen.")
	}
}

type aExtension struct {
	name      string
	isStopped bool
	ten.DefaultExtension
}

func (p *aExtension) OnDeinit(tenEnv ten.TenEnv) {
	defer tenEnv.OnDeinitDone()

	tenEnv.LogDebug("onDeinit")
	if !p.isStopped {
		panic("should not happen.")
	}
}

func (p *aExtension) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	go func() {
		cmdName, _ := cmd.GetName()
		tenEnv.LogInfo(
			"receive command: " + cmdName,
		)
		if cmdName == "start" {
			tenEnv.SendCmd(cmd, func(r ten.TenEnv, cs ten.CmdResult) {
				r.ReturnResultDirectly(cs)
			})
		}
	}()
}

func (p *aExtension) OnStop(tenEnv ten.TenEnv) {
	go func() {
		tenEnv.LogDebug("onStop ")

		cmd, _ := ten.NewCmd("stop")
		respChan := make(chan ten.CmdResult, 1)

		tenEnv.SendCmd(cmd, func(tenEnv ten.TenEnv, cmdStatus ten.CmdResult) {
			respChan <- cmdStatus
		})

		select {
		case resp := <-respChan:
			statusCode, _ := resp.GetStatusCode()
			if statusCode == ten.StatusCodeOk {
				p.isStopped = true
				tenEnv.OnStopDone()
			} else {
				panic("stop failed.")
			}
		}
	}()
}

type bExtension struct {
	ten.DefaultExtension
	stopChan chan struct{}
}

func NewBExtension() *bExtension {
	return &bExtension{
		stopChan: make(chan struct{}),
	}
}

func (p *bExtension) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	go func() {
		cmdName, _ := cmd.GetName()
		tenEnv.LogInfo(
			"receive command: " +
				cmdName,
		)
		if cmdName == "start" {
			tenEnv.SendCmd(cmd, func(r ten.TenEnv, cs ten.CmdResult) {
				r.ReturnResultDirectly(cs)
			})
		} else if cmdName == "stop" {
			tenEnv.SendCmd(cmd, func(r ten.TenEnv, cs ten.CmdResult) {
				r.ReturnResultDirectly(cs)

				close(p.stopChan)
				tenEnv.LogInfo("Stop command is processed.")
			})
		} else {
			cmdResult, _ := ten.NewCmdResult(ten.StatusCodeError)
			cmdResult.SetPropertyString("detail", "unknown cmd")
			tenEnv.ReturnResult(cmdResult, cmd)
		}
	}()
}

func (p *bExtension) OnStop(tenEnv ten.TenEnv) {
	go func() {
		tenEnv.LogDebug("OnStop")

		// Wait until the stop command is received and processed.
		<-p.stopChan

		tenEnv.LogInfo("Stop command processed. Now calling OnStopDone.")
		tenEnv.OnStopDone()
	}()
}

type cExtension struct {
	ten.DefaultExtension
	stopChan chan struct{}
}

func NewCExtension() *cExtension {
	return &cExtension{
		stopChan: make(chan struct{}),
	}
}

func (p *cExtension) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	go func() {
		cmdName, _ := cmd.GetName()
		tenEnv.LogInfo(
			"receive command: " +
				cmdName,
		)
		if cmdName == "start" {
			cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
			cmdResult.SetPropertyString("detail", "done")
			tenEnv.ReturnResult(cmdResult, cmd)
		} else if cmdName == "stop" {
			go func() {
				time.Sleep(time.Millisecond * 500)
				cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
				cmdResult.SetPropertyString("detail", "done")
				tenEnv.ReturnResult(cmdResult, cmd)

				tenEnv.LogInfo("Stop command is processed.")

				close(p.stopChan)
			}()
		} else {
			cmdResult, _ := ten.NewCmdResult(ten.StatusCodeError)
			cmdResult.SetPropertyString("detail", "unknown cmd")
			tenEnv.ReturnResult(cmdResult, cmd)
		}
	}()
}

func (p *cExtension) OnStop(tenEnv ten.TenEnv) {
	go func() {
		tenEnv.LogDebug("OnStop")

		// Wait until the stop command is received and processed.
		<-p.stopChan

		tenEnv.LogInfo("Stop command processed. Now calling OnStopDone.")
		tenEnv.OnStopDone()
	}()
}

func init() {
	fmt.Println("call init")

	// Register addon
	err := ten.RegisterAddonAsExtension("nodetest", &addonImpl{})
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
