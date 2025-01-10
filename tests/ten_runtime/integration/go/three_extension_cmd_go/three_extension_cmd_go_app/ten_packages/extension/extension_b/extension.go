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

type bExtension struct {
	aptima.DefaultExtension
}

func NewExtensionB(name string) aptima.Extension {
	return &bExtension{}
}

func (p *bExtension) OnCmd(
	tenEnv aptima.TenEnv,
	cmd aptima.Cmd,
) {
	go func() {
		cmdName, _ := cmd.GetName()
		fmt.Println(
			"bExtension receive  command: ",
			cmdName,
		)

		if cmdName == "B" {
			data, err := cmd.GetPropertyInt64("data")
			if err != nil {
				panic("Should not happen.")
			}

			cmdC, _ := aptima.NewCmd("C")
			err = cmdC.SetProperty("data", data*3)
			if err != nil {
				panic("Should not happen.")
			}

			err = tenEnv.SendCmd(
				cmdC,
				func(tenEnv aptima.TenEnv, cmdResult aptima.CmdResult, e error) {
					detail, err := cmdResult.GetPropertyString("detail")
					if err != nil {
						panic("Should not happen.")
					}
					statusCode, _ := cmdResult.GetStatusCode()
					tenEnv.LogInfo(
						"statusCode:" + fmt.Sprintf(
							"%d",
							statusCode,
						) + " detail: " + detail,
					)

					cmdResult2, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
					cmdResult2.SetPropertyString("detail", detail)
					err = tenEnv.ReturnResult(cmdResult2, cmd, nil)
					if err != nil {
						panic("Should not happen.")
					}
				},
			)
			if err != nil {
				panic("sendCmd failed")
			}
		} else {
		}
	}()
}

func init() {
	fmt.Println("call init")

	// Register addon
	err := aptima.RegisterAddonAsExtension(
		"extension_b",
		aptima.NewDefaultExtensionAddon(NewExtensionB),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
