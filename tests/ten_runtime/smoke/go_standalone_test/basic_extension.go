// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
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
