//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/TEN-framework/ten_framework/LICENSE for more
// information.
//

package default_extension_go

import (
	"fmt"

	"ten_framework/ten"
)

type cExtension struct {
	ten.DefaultExtension
}

func newCExtension(name string) ten.Extension {
	return &cExtension{}
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

		if cmdName == "C" {
			cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
			cmdResult.SetPropertyString("detail", "I am cExtension")
			err := tenEnv.ReturnResult(cmdResult, cmd)
			if err != nil {
				panic("should not happen")
			}
		}
	}()
}

func init() {
	fmt.Println("call init")

	// Register addon
	err := ten.RegisterAddonAsExtension(
		"extension_c",
		ten.NewDefaultExtensionAddon(newCExtension),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
