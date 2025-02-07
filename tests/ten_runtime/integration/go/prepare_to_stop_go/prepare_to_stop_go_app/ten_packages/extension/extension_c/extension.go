// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/APTIMA-framework/ten_framework/LICENSE for more
// information.
//
// Note that this is just an example extension written in the GO programming
// language, so the package name does not equal to the containing directory
// name. However, it is not common in Go.
package default_extension_go

import (
	"fmt"
	"time"

	"ten_framework/aptima"
)

type cExtension struct {
	aptima.DefaultExtension
	stopChan chan struct{}
}

func NewCExtension(name string) aptima.Extension {
	return &cExtension{
		stopChan: make(chan struct{}),
	}
}

func (p *cExtension) OnCmd(
	tenEnv aptima.TenEnv,
	cmd aptima.Cmd,
) {
	go func() {
		cmdName, _ := cmd.GetName()
		tenEnv.LogInfo(
			"receive command: " +
				cmdName,
		)
		if cmdName == "start" {
			cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
			cmdResult.SetPropertyString("detail", "done")
			tenEnv.ReturnResult(cmdResult, cmd, nil)
		} else if cmdName == "stop" {
			go func() {
				time.Sleep(time.Millisecond * 500)
				cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
				cmdResult.SetPropertyString("detail", "done")
				tenEnv.ReturnResult(cmdResult, cmd, nil)

				tenEnv.LogInfo("Stop command is processed.")

				close(p.stopChan)
			}()
		} else {
			cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeError)
			cmdResult.SetPropertyString("detail", "unknown cmd")
			tenEnv.ReturnResult(cmdResult, cmd, nil)
		}
	}()
}

func (p *cExtension) OnStop(tenEnv aptima.TenEnv) {
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
	err := aptima.RegisterAddonAsExtension(
		"extension_c",
		aptima.NewDefaultExtensionAddon(NewCExtension),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
