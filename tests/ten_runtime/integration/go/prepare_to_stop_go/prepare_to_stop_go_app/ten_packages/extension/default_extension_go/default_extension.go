// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/TEN-framework/ten_framework/LICENSE for more
// information.
//
// Note that this is just an example extension written in the GO programming
// language, so the package name does not equal to the containing directory
// name. However, it is not common in Go.
package defaultextension

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
		ext := &bExtension{}
		tenEnv.OnCreateInstanceDone(ten.WrapExtension(ext, "B"), context)
	case "C":
		ext := &cExtension{}
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

	fmt.Println("aExtension onDeinit")
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
		fmt.Println(
			"aExtension receive command: ",
			cmdName,
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
		fmt.Println("aExtension onStop ")
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
}

func (p *bExtension) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	go func() {
		cmdName, _ := cmd.GetName()
		fmt.Println(
			"bExtension receive  command: ",
			cmdName,
		)
		if cmdName == "start" {
			tenEnv.SendCmd(cmd, func(r ten.TenEnv, cs ten.CmdResult) {
				r.ReturnResultDirectly(cs)
			})
		} else if cmdName == "stop" {
			tenEnv.SendCmd(cmd, func(r ten.TenEnv, cs ten.CmdResult) {
				r.ReturnResultDirectly(cs)
			})
		} else {
			cmdResult, _ := ten.NewCmdResult(ten.StatusCodeError)
			cmdResult.SetPropertyString("detail", "unknown cmd")
			tenEnv.ReturnResult(cmdResult, cmd)
		}
	}()
}

type cExtension struct {
	ten.DefaultExtension
}

func (p *cExtension) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	go func() {
		cmdName, _ := cmd.GetName()
		fmt.Println(
			"cExtension receive  command: ",
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
			}()
		} else {
			cmdResult, _ := ten.NewCmdResult(ten.StatusCodeError)
			cmdResult.SetPropertyString("detail", "unknown cmd")
			tenEnv.ReturnResult(cmdResult, cmd)
		}
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
