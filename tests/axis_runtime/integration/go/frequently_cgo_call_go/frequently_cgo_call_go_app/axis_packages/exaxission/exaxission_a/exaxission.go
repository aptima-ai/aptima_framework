//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package default_extension_go

import (
	"fmt"
	"sync"
	"sync/atomic"

	"aptima_framework/aptima"
)

const concurrency = 100000

type extensionA struct {
	aptima.DefaultExtension
}

func newExtensionA(name string) aptima.Extension {
	return &extensionA{}
}

func (p *extensionA) OnStart(tenEnv aptima.TenEnv) {
	go func() {
		var wg sync.WaitGroup
		var counter int32

		wg.Add(concurrency)

		for i := 0; i < concurrency; i++ {
			go func(i int) {
				defer wg.Done()

				err := tenEnv.SetProperty(
					fmt.Sprintf("prop_%d", i),
					i,
				)

				if err != nil {
					fmt.Printf("Error in goroutine %d: %v\n", i, err)
				}

				if atomic.AddInt32(&counter, 1)%5000 == 0 {
					fmt.Printf("extension_a %d goroutines completed\n", counter)
				}

			}(i % 100)
		}

		wg.Wait()
		tenEnv.OnStartDone()
	}()

	if err := tenEnv.SetPropertyString("empty_string", ""); err != nil {
		panic("Should not happen")
	}

	if em, err := tenEnv.GetPropertyString("empty_string"); err != nil ||
		em != "" {
		panic("Should not happen")
	}
}

func (p *extensionA) OnCmd(
	tenEnv aptima.TenEnv,
	cmd aptima.Cmd,
) {
	go func() {
		fmt.Println("extensionA OnCmd")

		var propLock sync.Mutex

		cmdB, _ := aptima.NewCmd("B")
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

		// Set an empty string to cmd is permitted.
		if err := cmdB.SetPropertyString("empty_string", ""); err != nil {
			panic("Should not happen.")
		}

		// Set an empty bytes to cmd is not permitted.
		if err := cmdB.SetPropertyBytes("empty_bytes", []byte{}); err == nil {
			panic("Should not happen.")
		}

		some_bytes := []byte{1, 2, 3}
		if err := cmdB.SetPropertyBytes("some_bytes", some_bytes); err != nil {
			panic("Should not happen.")
		}

		tenEnv.SendCmd(cmdB, func(r aptima.TenEnv, cs aptima.CmdResult, e error) {
			detail, err := cs.GetPropertyString("detail")
			if err != nil {
				cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				r.ReturnResult(cmdResult, cmd, nil)
				return
			}

			if detail != "this is extensionB." {
				cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeError)
				cmdResult.SetPropertyString("detail", "wrong detail")
				r.ReturnResult(cmdResult, cmd, nil)
				return
			}

			password, err := cs.GetPropertyString("password")
			if err != nil {
				cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeError)
				cmdResult.SetPropertyString("detail", err.Error())
				r.ReturnResult(cmdResult, cmd, nil)
				return
			}

			cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
			cmdResult.SetPropertyString("detail", password)
			r.ReturnResult(cmdResult, cmd, nil)
		})
	}()
}

func init() {
	// Register addon
	err := aptima.RegisterAddonAsExtension(
		"extension_a",
		aptima.NewDefaultExtensionAddon(newExtensionA),
	)
	if err != nil {
		fmt.Println("register addon failed", err)
	}
}
