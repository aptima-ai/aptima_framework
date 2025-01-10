//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/aptima-ai/aptima_framework/LICENSE for more
// information.
//

package default_extension_go

import (
	"fmt"

	"aptima_framework/aptima"
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
			// Try to get nonexistent property.
			res, err := cmd.GetPropertyString("agora")
			if err != nil {
				cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				tenEnv.ReturnResult(cmdResult, cmd, nil)
			} else {
				cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
				cmdResult.SetPropertyString("detail", res)
				tenEnv.ReturnResult(cmdResult, cmd, nil)
			}

		} else {
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
