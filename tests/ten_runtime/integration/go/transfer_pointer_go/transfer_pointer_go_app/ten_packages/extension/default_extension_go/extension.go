// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/TEN-framework/ten_framework/LICENSE for more
// information.
//
// Note that this is just an example extension written in the GO programming
// language, so the package name does not equal to the containing directory
// name. However, it is not common in Go.
package default_extension_go

import (
	"fmt"

	"ten_framework/ten"
)

type extensionA struct {
	ten.DefaultExtension
}

type extensionB struct {
	ten.DefaultExtension
}

func newExtensionA(name string) ten.Extension {
	return &extensionA{}
}

func newExtensionB(name string) ten.Extension {
	return &extensionB{}
}

type userData struct {
	num int
	str string
}

func (p *extensionA) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	go func() {
		fmt.Println("extensionA OnCmd")

		cmdB, _ := ten.NewCmd("B")

		strArray := []string{"hello", "world", "ten"}
		cmdB.SetProperty("array", &strArray)

		propMap := make(map[string]interface{})
		propMap["paramA"] = "A"
		cmdB.SetProperty("map", &propMap)

		data := &userData{num: 2, str: "str"}
		cmdB.SetProperty("struct", data)
		data.num = 3

		tenEnv.SendCmd(cmdB, func(r ten.TenEnv, cs ten.CmdResult) {
			detail, err := cs.GetPropertyString("detail")
			if err != nil {
				cmdResult, _ := ten.NewCmdResult(ten.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				r.ReturnResult(cmdResult, cmd)
				return
			}

			cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
			cmdResult.SetPropertyString("detail", detail)
			r.ReturnResult(cmdResult, cmd)
		})
	}()
}

func (p *extensionB) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
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
				strArray[2] != "ten" {
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

			structData := structPtr.(*userData)

			if structData.num != 3 || structData.str != "str" {
				panic("should not happen")
			}

			cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
			cmdResult.SetPropertyString("detail", "ten")
			tenEnv.ReturnResult(cmdResult, cmd)
		}
	}()
}

func init() {
	// Register addon
	err := ten.RegisterAddonAsExtension(
		"extension_a",
		ten.NewDefaultExtensionAddon(newExtensionA),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}

	err = ten.RegisterAddonAsExtension(
		"extension_b",
		ten.NewDefaultExtensionAddon(newExtensionB),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
