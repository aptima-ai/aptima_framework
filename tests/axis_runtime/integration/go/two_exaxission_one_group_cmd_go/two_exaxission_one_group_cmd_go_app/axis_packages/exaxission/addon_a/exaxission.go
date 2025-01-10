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

	"aptima_framework/aptima"
)

type baseExtension struct {
	baseName  string
	baseCount int
	aptima.DefaultExtension
}

func (p *baseExtension) OnStart(tenEnv aptima.TenEnv) {
	fmt.Println("baseExtension onStart, name:", p.baseName)

	tenEnv.OnStartDone()
}

func (p *baseExtension) OnStop(tenEnv aptima.TenEnv) {
	fmt.Println("baseExtension OnStop name:", p.baseName)

	tenEnv.OnStopDone()
}

type aExtension struct {
	baseExtension
	aCount int
	aName  string
}

func NewAExtension(name string) aptima.Extension {
	return &aExtension{
		baseExtension: baseExtension{baseCount: 5, baseName: "aBase"},
		aCount:        4,
	}
}

func (p *aExtension) OnInit(
	tenEnv aptima.TenEnv,
) {
	p.aCount = p.aCount + 6
	p.aName = "after start"
	p.baseExtension.baseCount = 3

	tenEnv.OnInitDone()
}

func (p *aExtension) OnCmd(
	tenEnv aptima.TenEnv,
	cmd aptima.Cmd,
) {
	go func() {
		fmt.Println(
			"aExtension baseName : aBase = ",
			p.baseName,
			" 3 = ",
			p.baseCount,
			" 10 = ",
			p.aCount,
			" after start = ",
			p.aName,
		)
		cmdName, _ := cmd.GetName()
		fmt.Println(
			"aExtension receive command: ",
			cmdName,
		)

		data, _ := aptima.NewData("data")

		if err := data.SetPropertyString("empty_string", ""); err != nil {
			panic("the empty string is allowed")
		}

		if err := data.SetPropertyBytes("empty_bytes", []byte{}); err == nil {
			panic("the empty bytes is not allowed")
		}

		err := data.SetPropertyBytes("test_data_path", []byte(p.aName))
		if err != nil {
			panic("data SetPropertyBytes failed")
		}

		err = tenEnv.SendData(data, nil)
		if err != nil {
			panic("aExtension SendData failed")
		}

		cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
		cmdResult.SetPropertyString("detail", "world")
		err = tenEnv.ReturnResult(cmdResult, cmd, nil)
		if err != nil {
			panic("aExtension ReturnResult failed")
		}
	}()
}

func init() {
	// Register addon
	err := aptima.RegisterAddonAsExtension(
		"addon_a",
		aptima.NewDefaultExtensionAddon(NewAExtension),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
