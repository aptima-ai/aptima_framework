// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/aptima-ai/aptima_framework/LICENSE for more
// information.
//
// Note that this is just an example extension written in the GO programming
// language, so the package name does not equal to the containing directory
// name. However, it is not common in Go.
package default_extension_go

import (
	"fmt"

	"aptima_framework/aptima"
)

type aExtension struct {
	name      string
	isStopped bool
	aptima.DefaultExtension
}

func newAExtension(name string) aptima.Extension {
	return &aExtension{name: name, isStopped: false}
}

func (p *aExtension) OnDeinit(tenEnv aptima.TenEnv) {
	defer tenEnv.OnDeinitDone()

	tenEnv.LogDebug("onDeinit")
	if !p.isStopped {
		panic("should not happen.")
	}
}

func (p *aExtension) OnCmd(
	tenEnv aptima.TenEnv,
	cmd aptima.Cmd,
) {
	go func() {
		cmdName, _ := cmd.GetName()
		tenEnv.LogInfo(
			"receive command: " + cmdName,
		)
		if cmdName == "start" {
			tenEnv.SendCmd(cmd, func(r aptima.TenEnv, cs aptima.CmdResult, e error) {
				r.ReturnResultDirectly(cs, nil)
			})
		}
	}()
}

func (p *aExtension) OnStop(tenEnv aptima.TenEnv) {
	go func() {
		tenEnv.LogDebug("onStop ")

		cmd, _ := aptima.NewCmd("stop")
		respChan := make(chan aptima.CmdResult, 1)

		tenEnv.SendCmd(
			cmd,
			func(tenEnv aptima.TenEnv, cmdResult aptima.CmdResult, e error) {
				respChan <- cmdResult
			},
		)

		select {
		case resp := <-respChan:
			statusCode, _ := resp.GetStatusCode()
			if statusCode == aptima.StatusCodeOk {
				p.isStopped = true
				tenEnv.OnStopDone()
			} else {
				panic("stop failed.")
			}
		}
	}()
}

func init() {
	fmt.Println("call init")

	// Register addon
	err := aptima.RegisterAddonAsExtension(
		"extension_a",
		aptima.NewDefaultExtensionAddon(newAExtension),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
