//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
// Note that this is just an example extension written in the GO programming
// language, so the package name does not equal to the containing directory
// name. However, it is not common in Go.
//

package defaultextension

import (
	"fmt"

	"ten_framework/ten"
)

type defaultExtension struct {
	ten.DefaultExtension
}

func newDefaultExtension(name string) ten.Extension {
	return &defaultExtension{}
}

func (p *defaultExtension) OnStart(tenEnv ten.TenEnv) {
	tenEnv.LogDebug("OnStart")

	tenEnv.OnStartDone()
}

func (p *defaultExtension) OnStop(tenEnv ten.TenEnv) {
	tenEnv.LogDebug("OnStop")

	tenEnv.OnStopDone()
}

func (p *defaultExtension) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	tenEnv.LogDebug("OnCmd")

	cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
	cmdResult.SetPropertyString("detail", "This is default go extension.")
	tenEnv.ReturnResult(cmdResult, cmd)
}

func init() {
	fmt.Println("defaultExtension init")

	// Register addon
	ten.RegisterAddonAsExtension(
		"default_extension_go",
		ten.NewDefaultExtensionAddon(newDefaultExtension),
	)
}
