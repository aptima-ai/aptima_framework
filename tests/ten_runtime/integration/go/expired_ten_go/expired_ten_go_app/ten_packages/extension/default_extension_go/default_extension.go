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
	"time"

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
			detail, err := cs.GetPropertyPtr("data")
			if err != nil {
				cmdResult, _ := ten.NewCmdResult(ten.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				tenEnv.ReturnResult(cmdResult, cmd)
			} else {
				cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
				cmdResult.SetPropertyString("detail", detail.(*userData).name)
				tenEnv.ReturnResult(cmdResult, cmd)
			}
		})

		time.Sleep(time.Second * 3)
		// Use the expired ten object.
		err := tenEnv.SendCmd(cmdB, nil)
		if err != nil {
			fmt.Println("failed to use invalid ten object.")
		} else {
			panic("should not happen")
		}
	}()
}

type userData struct {
	uid  int
	name string
}

func (p *extensionB) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	go func() {
		fmt.Println("extensionB OnCmd")

		cmdName, _ := cmd.GetName()
		if cmdName == "B" {
			data := userData{uid: 1, name: "ten"}
			cs, _ := ten.NewCmdResult(ten.StatusCodeOk)
			cs.SetProperty("data", &data)
			err := tenEnv.ReturnResult(cs, cmd)
			if err != nil {
				panic(err)
			}
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
