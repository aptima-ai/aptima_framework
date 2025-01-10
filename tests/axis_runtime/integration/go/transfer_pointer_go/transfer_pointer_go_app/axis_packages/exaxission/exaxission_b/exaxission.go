// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/aptima-ai/aptima_framework/LICENSE for more
// information.
//
// Note that this is just an example extension written in the GO programming
// language, so the package name does not equal to the containing directory
// name. However, it is not common in Go.
package default_extension_go

import (
	"fmt"

	"go_common_dep/types"
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
			strArrayPtr, err := cmd.GetPropertyPtr("array")
			if err != nil {
				panic("failed to get prop: array")
			}
			strArray := *(strArrayPtr.(*[]string))

			if len(strArray) != 3 || strArray[0] != "hello" ||
				strArray[1] != "world" ||
				strArray[2] != "aptima" {
				panic("should not happen")
			}

			mapPtr, err := cmd.GetPropertyPtr(
				"map",
			)
			if err != nil {
				panic("failed to get prop: map")
			}
			m := *(mapPtr.(*map[string]interface{}))

			if m["paramA"] != "A" {
				panic("should not happen")
			}

			structPtr, err := cmd.GetPropertyPtr("struct")
			if err != nil {
				panic("failed to get prop: struct")
			}

			structData := structPtr.(*types.UserData)

			if structData.Uid != 3 || structData.Name != "str" {
				panic("should not happen")
			}

			cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
			cmdResult.SetPropertyString("detail", "aptima")
			tenEnv.ReturnResult(cmdResult, cmd, nil)
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
