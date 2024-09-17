// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/TEN-framework/ten_framework/LICENSE for more
// information.
//
// Note that this is just an example extension written in the GO programming
// language, so the package name does not equal to the containing directory
// name. However, it is not common in Go.
package defaultextension

import (
	"fmt"

	"ten_framework/ten"
)

type baseExtension struct {
	baseName  string
	baseCount int
	ten.DefaultExtension
}

func (p *baseExtension) OnStart(tenEnv ten.TenEnv) {
	fmt.Println("baseExtension onStart, name:", p.baseName)

	tenEnv.OnStartDone()
}

func (p *baseExtension) OnStop(tenEnv ten.TenEnv) {
	fmt.Println("baseExtension OnStop name:", p.baseName)

	tenEnv.OnStopDone()
}

type aExtension struct {
	baseExtension
	aCount int
	aName  string
}

func NewAExtension(name string) ten.Extension {
	return &aExtension{
		baseExtension: baseExtension{baseCount: 5, baseName: "aBase"},
		aCount:        4,
	}
}

func (p *aExtension) OnInit(
	tenEnv ten.TenEnv,
) {
	p.aCount = p.aCount + 6
	p.aName = "after start"
	p.baseExtension.baseCount = 3

	tenEnv.OnInitDone()
}

func (p *aExtension) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
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

		data, _ := ten.NewData("data")

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

		err = tenEnv.SendData(data)
		if err != nil {
			panic("aExtension SendData failed")
		}

		cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
		cmdResult.SetPropertyString("detail", "world")
		err = tenEnv.ReturnResult(cmdResult, cmd)
		if err != nil {
			panic("aExtension ReturnResult failed")
		}
	}()
}

type bExtension struct {
	baseExtension
	bName string
}

func NewBExtension(name string) ten.Extension {
	return &bExtension{
		baseExtension: baseExtension{baseCount: 1, baseName: "bBase"},
		bName:         "before_start",
	}
}

func (p *bExtension) OnData(
	tenEnv ten.TenEnv,
	data ten.Data,
) {
	fmt.Println(
		"bExtension onData, bName: before_start = ",
		p.bName,
		" baseName: bBase = ",
		p.baseName,
	)

	// The buffer in data is always copied to GO.
	bytes, err := data.GetPropertyBytes("test_data_path")
	if err != nil {
		panic(err)
	}

	defer ten.ReleaseBytes(bytes)

	res := string(bytes)
	if res != "after start" {
		panic("wrong in bExtension OnData")
	}
}

type nodeTestGroup struct {
	ten.DefaultExtensionGroup
}

func newNodeTestGroup(name string) ten.ExtensionGroup {
	return &nodeTestGroup{}
}

func (p *nodeTestGroup) OnCreateExtensions(
	tenEnv ten.TenEnv,
) {
	var extensionsArray []ten.Extension

	callback := func(tenEnv ten.TenEnv, p ten.Extension) {
		extensionsArray = append(extensionsArray, p)
		if len(extensionsArray) == 2 {
			tenEnv.OnCreateExtensionsDone(extensionsArray...)
		}
	}

	err := tenEnv.AddonCreateExtensionAsync("addon_a", "A", callback)
	if err != nil {
		panic("should not happen")
	}

	err = tenEnv.AddonCreateExtensionAsync("addon_b", "B", callback)
	if err != nil {
		panic("should not happen")
	}
}

func (p *nodeTestGroup) OnDestroyExtensions(
	tenEnv ten.TenEnv,
	extensions []ten.Extension,
) {
	count := 0
	callback := func(tenEnv ten.TenEnv) {
		count += 1
		if count == 2 {
			tenEnv.OnDestroyExtensionsDone()
		}
	}

	for _, extension := range extensions {
		err := tenEnv.AddonDestroyExtensionAsync(extension, callback)
		if err != nil {
			panic("should not happen")
		}
	}
}

func init() {
	// Register addon
	err := ten.RegisterAddonAsExtensionGroup(
		"nodetest",
		ten.NewDefaultExtensionGroupAddon(newNodeTestGroup),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}

	err = ten.RegisterAddonAsExtension(
		"addon_a",
		ten.NewDefaultExtensionAddon(NewAExtension),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}

	err = ten.RegisterAddonAsExtension(
		"addon_b",
		ten.NewDefaultExtensionAddon(NewBExtension),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
