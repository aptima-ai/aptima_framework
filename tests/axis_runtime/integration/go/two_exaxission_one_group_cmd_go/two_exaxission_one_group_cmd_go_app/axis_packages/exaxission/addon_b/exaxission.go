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

type bExtension struct {
	baseExtension
	bName string
}

func NewBExtension(name string) aptima.Extension {
	return &bExtension{
		baseExtension: baseExtension{baseCount: 1, baseName: "bBase"},
		bName:         "before_start",
	}
}

func (p *bExtension) OnData(
	tenEnv aptima.TenEnv,
	data aptima.Data,
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

	defer aptima.ReleaseBytes(bytes)

	res := string(bytes)
	if res != "after start" {
		panic("wrong in bExtension OnData")
	}
}

func init() {
	// Register addon
	err := aptima.RegisterAddonAsExtension(
		"addon_b",
		aptima.NewDefaultExtensionAddon(NewBExtension),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
