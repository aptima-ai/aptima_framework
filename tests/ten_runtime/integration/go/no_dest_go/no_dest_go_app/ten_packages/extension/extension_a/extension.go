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

func (ext *aExtension) OnStart(tenEnv ten.TenEnv) {
	tenEnv.LogDebug("OnStart")

	noDestCmd, _ := ten.NewCmd("unknownCmd")
	tenEnv.SendCmd(noDestCmd, func(te ten.TenEnv, cr ten.CmdResult, err error) {
		if err == nil {
			panic("SendCmd should fail if no destination is found.")
		}

		tenEnv.LogInfo("SendCmd failed as expected, err: " + err.Error())

		ext.counter++

		if ext.counter == 4 {
			tenEnv.OnStartDone()
		}
	})

	noDestData, _ := ten.NewData("unknownData")
	tenEnv.SendData(noDestData, func(te ten.TenEnv, err error) {
		if err == nil {
			panic("SendData should fail if no destination is found.")
		}

		tenEnv.LogInfo("SendData failed as expected, err: " + err.Error())

		ext.counter++

		if ext.counter == 4 {
			tenEnv.OnStartDone()
		}
	})

	noDestVideoFrame, _ := ten.NewVideoFrame("unknownVideoFrame")
	tenEnv.SendVideoFrame(noDestVideoFrame, func(te ten.TenEnv, err error) {
		if err == nil {
			panic("SendVideoFrame should fail if no destination is found.")
		}

		tenEnv.LogInfo("SendVideoFrame failed as expected, err: " + err.Error())

		ext.counter++

		if ext.counter == 4 {
			tenEnv.OnStartDone()
		}
	})

	noDestAudioFrame, _ := ten.NewAudioFrame("unknownAudioFrame")
	tenEnv.SendAudioFrame(noDestAudioFrame, func(te ten.TenEnv, err error) {
		if err == nil {
			panic("SendAudioFrame should fail if no destination is found.")
		}

		tenEnv.LogInfo("SendAudioFrame failed as expected, err: " + err.Error())

		ext.counter++

		if ext.counter == 4 {
			tenEnv.OnStartDone()
		}
	})
}

func (ext *aExtension) OnStop(tenEnv ten.TenEnv) {
	tenEnv.LogDebug("OnStop")

	tenEnv.OnStopDone()
}

type aExtension struct {
	baseExtension

	counter int
}

func newAExtension(name string) ten.Extension {
	return &aExtension{counter: 0}
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
