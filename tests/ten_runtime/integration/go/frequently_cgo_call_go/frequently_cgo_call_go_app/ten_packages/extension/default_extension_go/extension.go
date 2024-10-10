//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package default_extension_go

import (
	"fmt"
	"sync"
	"sync/atomic"

	"ten_framework/ten"
)

const concurrency = 10000

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

func (p *extensionA) OnStart(tenEnv ten.TenEnv) {
	count := 0
	for i := 0; i < concurrency; i++ {
		go func(i int) {
			tenEnv.SetPropertyAsync(
				fmt.Sprintf("prop_%d", i),
				i,
				func(r ten.TenEnv, e error) {
					// It's thread safe because the callback is always called on
					// the extension thread.
					count++
					if count == concurrency {
						tenEnv.OnStartDone()
					}
				},
			)
		}(i % 100)
	}
}

func (p *extensionA) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	go func() {
		fmt.Println("extensionA OnCmd")

		var propLock sync.Mutex

		cmdB, _ := ten.NewCmd("B")
		var count uint32 = 0

		done := make(chan struct{}, 1)
		defer close(done)

		for i := 0; i < concurrency; i++ {
			go func(i int) {
				propLock.Lock()

				// The SetProperty is not goroutine safety.
				err := cmdB.SetProperty(fmt.Sprintf("prop_%d", i), i)
				propLock.Unlock()

				if err != nil {
					panic("should not happen")
				}

				cmdName, _ := cmdB.GetName()
				if cmdName != "B" {
					panic("should not happen")
				}

				if atomic.AddUint32(&count, 1) == concurrency {
					done <- struct{}{}
				}
			}(i % 100)
		}
		<-done

		tenEnv.SendCmd(cmdB, func(r ten.TenEnv, cs ten.CmdResult) {
			detail, err := cs.GetPropertyString("detail")
			if err != nil {
				cmdResult, _ := ten.NewCmdResult(ten.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				r.ReturnResult(cmdResult, cmd)
				return
			}

			if detail != "this is extensionB." {
				cmdResult, _ := ten.NewCmdResult(ten.StatusCodeError)
				cmdResult.SetPropertyString("detail", "wrong detail")
				r.ReturnResult(cmdResult, cmd)
				return
			}

			password, err := cs.GetPropertyString("password")
			if err != nil {
				cmdResult, _ := ten.NewCmdResult(ten.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				r.ReturnResult(cmdResult, cmd)
				return
			}

			cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
			cmdResult.SetPropertyString("detail", password)
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

		connected, err := tenEnv.IsCmdConnected("cmd_not_exist")
		if err != nil || connected {
			panic("Should not happen.")
		}

		cmdName, _ := cmd.GetName()
		if cmdName == "B" {
			var count uint32 = 0

			done := make(chan struct{}, 1)
			defer close(done)

			for i := 0; i < concurrency; i++ {
				go func(i int) {
					res, err := cmd.GetPropertyInt64(
						fmt.Sprintf("prop_%d", i),
					)
					if err != nil || int(res) != i {
						panic("should not happen")
					}

					cmdName, _ := cmd.GetName()
					if cmdName != "B" {
						panic("should not happen")
					}

					if atomic.AddUint32(&count, 1) == concurrency {
						done <- struct{}{}
					}
				}(i % 100)
			}
			<-done

			statusCmd, err := ten.NewCmdResult(
				ten.StatusCodeOk,
			)
			if err != nil {
				cmdResult, _ := ten.NewCmdResult(ten.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				tenEnv.ReturnResult(cmdResult, cmd)
				return
			}

			statusCmd.SetProperty("detail", "this is extensionB.")
			statusCmd.SetProperty("password", "password")
			tenEnv.ReturnResult(statusCmd, cmd)
		} else {
			cmdResult, _ := ten.NewCmdResult(ten.StatusCodeError)
			cmdResult.SetPropertyString("detail", "wrong cmd name")
			tenEnv.ReturnResult(cmdResult, cmd)
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
