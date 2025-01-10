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
	"time"

	"go_common_dep/types"
	"ten_framework/aptima"
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
			detail, err := cs.GetPropertyPtr("data")
			if err != nil {
				cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				tenEnv.ReturnResult(cmdResult, cmd, nil)
			} else {
				cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
				cmdResult.SetPropertyString("detail", detail.(*types.UserData).Name)
				tenEnv.ReturnResult(cmdResult, cmd, nil)
			}
		})

		time.Sleep(time.Second * 3)
		// Use the expired aptima object.
		err := tenEnv.SendCmd(cmdB, nil)
		if err != nil {
			fmt.Println("failed to use invalid aptima object.")
		} else {
			panic("should not happen")
		}
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
