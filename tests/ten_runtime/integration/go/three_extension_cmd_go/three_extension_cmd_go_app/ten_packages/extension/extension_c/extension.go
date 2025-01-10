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
	"encoding/json"
	"fmt"

	"ten_framework/aptima"
)

type userData struct {
	Result int `json:"result"`
}

type cExtension struct {
	aptima.DefaultExtension
}

func NewExtensionC(name string) aptima.Extension {
	return &cExtension{}
}

func (p *cExtension) OnCmd(
	tenEnv aptima.TenEnv,
	cmd aptima.Cmd,
) {
	go func() {
		cmdName, _ := cmd.GetName()
		fmt.Println(
			"bExtension receive  command: ",
			cmdName,
		)

		if cmdName == "C" {
			data, err := cmd.GetPropertyInt64("data")
			if err != nil {
				panic("Should not happen.")
			}

			result := int(data) * 5

			res, _ := json.Marshal(userData{Result: result})

			fmt.Println("return command C, res:", string(res))

			cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
			cmdResult.SetPropertyString("detail", string(res))
			err = tenEnv.ReturnResult(cmdResult, cmd, nil)
			if err != nil {
				panic("Should not happen.")
			}

		} else {
		}
	}()
}

func init() {
	fmt.Println("call init")

	// Register addon
	err := aptima.RegisterAddonAsExtension(
		"extension_c",
		aptima.NewDefaultExtensionAddon(NewExtensionC),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
