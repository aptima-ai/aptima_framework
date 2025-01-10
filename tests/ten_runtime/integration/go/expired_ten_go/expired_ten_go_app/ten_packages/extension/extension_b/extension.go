//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/APTIMA-framework/ten_framework/LICENSE for more
// information.
//

package default_extension_go

import (
	"fmt"

	"go_common_dep/types"
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
			data := types.UserData{Uid: 1, Name: "aptima"}
			cs, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
			cs.SetProperty("data", &data)
			err := tenEnv.ReturnResult(cs, cmd, nil)
			if err != nil {
				panic(err)
			}
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
