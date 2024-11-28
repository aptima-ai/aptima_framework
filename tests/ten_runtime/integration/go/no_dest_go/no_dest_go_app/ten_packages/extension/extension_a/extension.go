//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/TEN-framework/ten_framework/LICENSE for more
// information.
//

package default_extension_go

import (
	"ten_framework/ten"
)

type baseExtension struct {
	ten.DefaultExtension
}

func (ext *baseExtension) OnStart(tenEnv ten.TenEnv) {
	tenEnv.LogDebug("OnStart")

	noDestCmd, _ := ten.NewCmd("unknown")
	tenEnv.SendCmd(noDestCmd, func(te ten.TenEnv, cr ten.CmdResult, err error) {
		if err == nil {
			panic("SendCmd should fail if no destination is found.")
		}

		tenEnv.LogInfo("SendCmd failed as expected, err: " + err.Error())

		tenEnv.OnStartDone()
	})
}

func (ext *baseExtension) OnStop(tenEnv ten.TenEnv) {
	tenEnv.LogDebug("OnStop")

	tenEnv.OnStopDone()
}

type aExtension struct {
	baseExtension
}

func newAExtension(name string) ten.Extension {
	return &aExtension{}
}

func (p *aExtension) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
	cmdResult.SetPropertyString("detail", "okok")
	err := tenEnv.ReturnResult(cmdResult, cmd)
	if err != nil {
		panic("ReturnResult failed")
	}
}

func init() {
	// Register addon
	err := ten.RegisterAddonAsExtension(
		"extension_a",
		ten.NewDefaultExtensionAddon(newAExtension),
	)
	if err != nil {
		panic("Failed to register addon.")
	}
}
