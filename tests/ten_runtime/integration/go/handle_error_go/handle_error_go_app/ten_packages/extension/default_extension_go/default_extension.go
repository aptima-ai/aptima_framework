// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
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

func (p *extensionA) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	go func() {
		fmt.Println("extensionA OnCmd")

		cmdB, _ := ten.NewCmd("B")
		tenEnv.SendCmd(cmdB, func(r ten.TenEnv, cs ten.CmdResult) {
			detail, err := cs.GetPropertyString("detail")
			if err != nil {
				cmdResult, _ := ten.NewCmdResult(ten.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				r.ReturnResult(cmdResult, cmd)
				return
			}

			// Try to get nonexistent property.
			_, err = tenEnv.GetPropertyString("agora")
			if err != nil {
				fmt.Println("getProp agora error, ", err)
			}

			_, err = tenEnv.GetPropertyString("agora")
			if err == nil {
				panic("should not happen")
			}

			fmt.Println("getPropertyAsync agora error, ", err)
			statusCode, _ := cs.GetStatusCode()
			cmdResult, _ := ten.NewCmdResult(statusCode)
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
			// Try to get nonexistent property.
			res, err := cmd.GetPropertyString("agora")
			if err != nil {
				cmdResult, _ := ten.NewCmdResult(ten.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				tenEnv.ReturnResult(cmdResult, cmd)
			} else {
				cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
				cmdResult.SetPropertyString("detail", res)
				tenEnv.ReturnResult(cmdResult, cmd)
			}

		} else {
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
