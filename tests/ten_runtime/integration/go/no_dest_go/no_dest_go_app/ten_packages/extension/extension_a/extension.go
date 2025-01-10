//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/APTIMA-framework/ten_framework/LICENSE for more
// information.
//

package default_extension_go

import (
	"ten_framework/aptima"
)

type baseExtension struct {
	aptima.DefaultExtension
}

func (ext *aExtension) OnStart(tenEnv aptima.TenEnv) {
	tenEnv.LogDebug("OnStart")

	noDestCmd, _ := aptima.NewCmd("unknownCmd")
	tenEnv.SendCmd(noDestCmd, func(te aptima.TenEnv, cr aptima.CmdResult, err error) {
		if err == nil {
			panic("SendCmd should fail if no destination is found.")
		}

		tenEnv.LogInfo("SendCmd failed as expected, err: " + err.Error())

		ext.counter++

		if ext.counter == 4 {
			tenEnv.OnStartDone()
		}
	})

	noDestData, _ := aptima.NewData("unknownData")
	tenEnv.SendData(noDestData, func(te aptima.TenEnv, err error) {
		if err == nil {
			panic("SendData should fail if no destination is found.")
		}

		tenEnv.LogInfo("SendData failed as expected, err: " + err.Error())

		ext.counter++

		if ext.counter == 4 {
			tenEnv.OnStartDone()
		}
	})

	noDestVideoFrame, _ := aptima.NewVideoFrame("unknownVideoFrame")
	tenEnv.SendVideoFrame(noDestVideoFrame, func(te aptima.TenEnv, err error) {
		if err == nil {
			panic("SendVideoFrame should fail if no destination is found.")
		}

		tenEnv.LogInfo("SendVideoFrame failed as expected, err: " + err.Error())

		ext.counter++

		if ext.counter == 4 {
			tenEnv.OnStartDone()
		}
	})

	noDestAudioFrame, _ := aptima.NewAudioFrame("unknownAudioFrame")
	tenEnv.SendAudioFrame(noDestAudioFrame, func(te aptima.TenEnv, err error) {
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

func (ext *aExtension) OnStop(tenEnv aptima.TenEnv) {
	tenEnv.LogDebug("OnStop")

	tenEnv.OnStopDone()
}

type aExtension struct {
	baseExtension

	counter int
}

func newAExtension(name string) aptima.Extension {
	return &aExtension{counter: 0}
}

func (p *aExtension) OnCmd(
	tenEnv aptima.TenEnv,
	cmd aptima.Cmd,
) {
	cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
	cmdResult.SetPropertyString("detail", "okok")
	err := tenEnv.ReturnResult(cmdResult, cmd, nil)
	if err != nil {
		panic("ReturnResult failed")
	}
}

func init() {
	// Register addon
	err := aptima.RegisterAddonAsExtension(
		"extension_a",
		aptima.NewDefaultExtensionAddon(newAExtension),
	)
	if err != nil {
		panic("Failed to register addon.")
	}
}
