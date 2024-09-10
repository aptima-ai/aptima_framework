// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
// Note that this is just an example extension written in the GO programming
// language, so the package name does not equal to the containing directory
// name. However, it is not common in Go.
package defaultextension

import (
	"encoding/json"
	"fmt"

	"ten_framework/ten"
)

type userData struct {
	Result int `json:"result"`
}

type addonImpl struct {
	ten.DefaultAddon
}

func (p *addonImpl) OnCreateInstance(
	tenEnv ten.TenEnv,
	name string,
	context uintptr,
) {
	switch name {
	case "A":
		tenEnv.OnCreateInstanceDone(
			ten.WrapExtension(&aExtension{name: name}, name), context,
		)
	case "B":
		tenEnv.OnCreateInstanceDone(
			ten.WrapExtension(&bExtension{}, name),
			context,
		)
	case "C":
		tenEnv.OnCreateInstanceDone(
			ten.WrapExtension(&cExtension{}, name),
			context,
		)
	default:
		fmt.Println("Bad name")
	}
}

type aExtension struct {
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
		if cmd.GetType() != ten.MsgTypeCmdResult {
			fmt.Println(p.name, "onCmd: ", cmdName)
		}

		if cmdName == "A" {
			cmdB, _ := ten.NewCmd("B")
			err := cmdB.SetProperty("data", 2)
			if err != nil {
				panic("Should not happen.")
			}
			err = tenEnv.SendCmd(
				cmdB,
				func(tenEnv ten.TenEnv, cmdStatus ten.CmdResult) {
					detail, err := cmdStatus.GetPropertyString("detail")
					if err != nil {
						panic("Should not happen.")
					}
					statusCode, _ := cmdStatus.GetStatusCode()
					fmt.Println(
						"statusCode:",
						statusCode,
						" detail: ",
						detail,
					)

					cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
					cmdResult.SetPropertyString("detail", detail)
					err = tenEnv.ReturnResult(cmdResult, cmd)
					if err != nil {
						panic("Should not happen.")
					}
				},
			)
			if err != nil {
				panic("sendCmd failed")
			}
		} else {
		}
	}()
}

func (p *aExtension) OnData(
	tenEnv ten.TenEnv,
	data ten.Data,
) {
	fmt.Println("aExtension onData")
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
			data, err := cmd.GetPropertyInt64("data")
			if err != nil {
				panic("Should not happen.")
			}

			cmdC, _ := ten.NewCmd("C")
			err = cmdC.SetProperty("data", data*3)
			if err != nil {
				panic("Should not happen.")
			}

			err = tenEnv.SendCmd(
				cmdC,
				func(tenEnv ten.TenEnv, cmdStatus ten.CmdResult) {
					detail, err := cmdStatus.GetPropertyString("detail")
					if err != nil {
						panic("Should not happen.")
					}
					statusCode, _ := cmdStatus.GetStatusCode()
					fmt.Println(
						"statusCode:",
						statusCode,
						" detail: ",
						detail,
					)

					cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
					cmdResult.SetPropertyString("detail", detail)
					err = tenEnv.ReturnResult(cmdResult, cmd)
					if err != nil {
						panic("Should not happen.")
					}
				},
			)
			if err != nil {
				panic("sendCmd failed")
			}
		} else {
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
			"bExtension receive  command: ",
			cmdName,
		)

		if cmdName == "C" {
			data, err := cmd.GetPropertyInt64("data")
			if err != nil {
				panic("Should not happen.")
			}

			result := int(data) * 5

			res, _ := json.Marshal(userData{Result: result})

			fmt.Println("return command C, res:", string(res))

			cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
			cmdResult.SetPropertyString("detail", string(res))
			err = tenEnv.ReturnResult(cmdResult, cmd)
			if err != nil {
				panic("Should not happen.")
			}

		} else {
		}
	}()
}

func init() {
	fmt.Println("call init")

	// Register addon
	err := ten.RegisterAddonAsExtension("nodetest", &addonImpl{})
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
