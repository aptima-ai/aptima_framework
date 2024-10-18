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

type aExtension struct {
	aCmd ten.Cmd
	name string
	ten.DefaultExtension
}

func newAExtension(name string) ten.Extension {
	return &aExtension{name: name}
}

func (p *aExtension) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	go func() {
		cmdName, _ := cmd.GetName()
		fmt.Println(
			"aExtension receive command: ",
			cmdName,
		)

		if cmdName == "A" {
			// Cache the cmd for returning it in the future.
			p.aCmd = cmd
			err := tenEnv.SendJSON(`{
				"_ten": {
				  "name": "B"
				}
			  }`, func(tenEnv ten.TenEnv, cmdStatus ten.CmdResult) {
				detail, err := cmdStatus.GetPropertyString("detail")
				if err != nil {
					panic("Should not happen.")
				}

				cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
				cmdResult.SetPropertyString("detail", detail)
				err = tenEnv.ReturnResult(cmdResult, p.aCmd)
				if err != nil {
					panic("Should not happen.")
				}
			})
			if err != nil {
				panic("aExtension sendCmd B failed")
			}

			return
		}
	}()
}

func init() {
	fmt.Println("call init")

	// Register addon
	err := ten.RegisterAddonAsExtension(
		"extension_a",
		ten.NewDefaultExtensionAddon(newAExtension),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
