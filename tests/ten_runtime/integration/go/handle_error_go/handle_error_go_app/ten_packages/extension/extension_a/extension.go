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
			detail, err := cs.GetPropertyString("detail")
			if err != nil {
				cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				r.ReturnResult(cmdResult, cmd, nil)
				return
			}

			// Try to get nonexistent property.
			_, err = tenEnv.GetPropertyString("agora")
			if err != nil {
				fmt.Println("getProp agora error, ", err)
			}

			_, err = tenEnv.GetPropertyString("agora")
			if err == nil {
				panic("should not happen")
			}

			fmt.Println("GetPropertyString error, ", err)

			statusCode, _ := cs.GetStatusCode()
			cmdResult, _ := aptima.NewCmdResult(statusCode)
			cmdResult.SetPropertyString("detail", detail)
			r.ReturnResult(cmdResult, cmd, nil)
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
