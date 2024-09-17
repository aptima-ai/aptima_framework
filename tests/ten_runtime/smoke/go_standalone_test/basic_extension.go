// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/TEN-framework/ten_framework/LICENSE for more
// information.
package standalone

import (
	"ten_framework/ten"
)

type basicExtension struct {
	ten.DefaultExtension
}

func newBasicExtension(name string) ten.Extension {
	return &basicExtension{}
}

func (p *basicExtension) OnCmd(tenEnv ten.TenEnv, cmd ten.Cmd) {
	cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
	cmdResult.SetPropertyString("detail", "success")
	tenEnv.ReturnResult(cmdResult, cmd)
}
