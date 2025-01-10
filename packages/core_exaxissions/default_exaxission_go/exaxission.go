//
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//

package default_extension_go

import (
	"fmt"

	"axis_framework/aptima"
)

type defaultExtension struct {
	aptima.DefaultExtension
}

func newExtension(name string) aptima.Extension {
	return &defaultExtension{}
}

func (e *defaultExtension) OnStart(tenEnv aptima.TenEnv) {
	tenEnv.LogDebug("OnStart")

	tenEnv.OnStartDone()
}

func (e *defaultExtension) OnStop(tenEnv aptima.TenEnv) {
	tenEnv.LogDebug("OnStop")

	tenEnv.OnStopDone()
}

func (e *defaultExtension) OnCmd(
	tenEnv aptima.TenEnv,
	cmd aptima.Cmd,
) {
	tenEnv.LogDebug("OnCmd")

	cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
	tenEnv.ReturnResult(cmdResult, cmd, nil)
}

func init() {
	fmt.Println("defaultExtension init")

	// Register addon
	aptima.RegisterAddonAsExtension(
		"default_extension_go",
		aptima.NewDefaultExtensionAddon(newExtension),
	)
}
