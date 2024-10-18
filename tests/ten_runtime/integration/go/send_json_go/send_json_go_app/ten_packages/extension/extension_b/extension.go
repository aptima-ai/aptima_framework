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

type bExtension struct {
	ten.DefaultExtension
}

func newBExtension(name string) ten.Extension {
	return &bExtension{}
}

func (p *bExtension) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	go func() {
		cmdName, _ := cmd.GetName()
		fmt.Println(
			"bExtension receive  command: ",
			cmdName,
		)

		if cmdName == "B" {
			err := tenEnv.SendJSON(`{
				"_ten": {
				  "name": "C"
				}
			  }`, func(tenEnv ten.TenEnv, cmdStatus ten.CmdResult) {
				detail, err := cmdStatus.GetPropertyString("detail")
				if err != nil {
					panic("should not happen")
				}

				cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
				cmdResult.SetPropertyString("detail", detail)
				err = tenEnv.ReturnResult(cmdResult, cmd)
				if err != nil {
					panic("should not happen")
				}
			})
			if err != nil {
				panic("bExtension sendCmd C failed")
			}

			return
		}
	}()
}

func init() {
	fmt.Println("call init")

	// Register addon
	err := ten.RegisterAddonAsExtension(
		"extension_b",
		ten.NewDefaultExtensionAddon(newBExtension),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
