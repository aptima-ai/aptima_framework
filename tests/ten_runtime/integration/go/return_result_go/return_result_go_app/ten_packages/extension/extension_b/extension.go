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

	"ten_framework/aptima"
)

type extensionB struct {
	aptima.DefaultExtension
}

func newExtensionB(name string) aptima.Extension {
	return &extensionB{}
}

func (p *extensionB) OnCmd(
	tenEnv aptima.TenEnv,
	cmd aptima.Cmd,
) {
	go func() {
		fmt.Println("extensionB OnCmd")

		cmdName, _ := cmd.GetName()
		if cmdName == "B" {
			statusCmd, err := aptima.NewCmdResult(
				aptima.StatusCodeOk,
			)
			if err != nil {
				cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				tenEnv.ReturnResult(cmdResult, cmd, nil)
				return
			}

			statusCmd.SetProperty("detail", "this is extensionB.")
			statusCmd.SetProperty("password", "password")
			tenEnv.ReturnResult(statusCmd, cmd, nil)
		}
	}()
}

func init() {
	// Register addon
	err := aptima.RegisterAddonAsExtension(
		"extension_b",
		aptima.NewDefaultExtensionAddon(newExtensionB),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
