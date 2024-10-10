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

type addonImpl struct {
	ten.DefaultAddon
}

func (p *addonImpl) OnCreateInstance(
	tenEnv ten.TenEnv, name string, context uintptr) {
	switch name {
	case "A":
		ext := &aExtension{name: name}
		tenEnv.OnCreateInstanceDone(ten.WrapExtension(ext, "A"), context)
	case "B":
		ext := &bExtension{}
		tenEnv.OnCreateInstanceDone(ten.WrapExtension(ext, "B"), context)
	case "C":
		ext := &cExtension{}
		tenEnv.OnCreateInstanceDone(ten.WrapExtension(ext, "C"), context)
	default:
		panic("Should not happen.")
	}
}

type aExtension struct {
	aCmd ten.Cmd
	name string
	ten.DefaultExtension
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

type bExtension struct {
	ten.DefaultExtension
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

type cExtension struct {
	ten.DefaultExtension
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
		"nodetest",
		&addonImpl{},
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
