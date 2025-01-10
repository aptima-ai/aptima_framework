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

type extensionA struct {
	aptima.DefaultExtension
}

func newExtensionA(name string) aptima.Extension {
	return &extensionA{}
}

func (p *extensionA) OnCmd(
	tenEnv aptima.TenEnv,
	cmd aptima.Cmd,
) {
	go func() {
		fmt.Println("extensionA OnCmd")

		cmdB, _ := aptima.NewCmd("B")
		tenEnv.SendCmd(cmdB, func(r aptima.TenEnv, cs aptima.CmdResult, e error) {
			detail, err := cs.GetPropertyString("detail")
			if err != nil {
				cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				tenEnv.ReturnResult(cmdResult, cmd, nil)
				return
			}

			if detail != "this is extensionB." {
				cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeError)
				cmdResult.SetPropertyString("detail", "wrong detail")
				tenEnv.ReturnResult(cmdResult, cmd, nil)
				return
			}

			password, err := cs.GetPropertyString("password")
			if err != nil {
				cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				tenEnv.ReturnResult(cmdResult, cmd, nil)
				return
			}

			cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
			cmdResult.SetPropertyString("detail", password)
			tenEnv.ReturnResult(cmdResult, cmd, nil)
		})
	}()
}

func init() {
	// Register addon
	err := aptima.RegisterAddonAsExtension(
		"extension_a",
		aptima.NewDefaultExtensionAddon(newExtensionA),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
