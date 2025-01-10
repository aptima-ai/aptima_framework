// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/aptima-ai/aptima_framework/LICENSE for more
// information.
package main

import (
	"fmt"
	"time"

	"aptima_framework/aptima"
)

type defaultApp struct {
	aptima.DefaultApp
}

func (p *defaultApp) OnDeinit(tenEnv aptima.TenEnv) {
	fmt.Println("DefaultApp onDeinit")

	tenEnv.OnDeinitDone()
}

func appRoutine(app aptima.App, stopped chan<- struct{}) {
	app.Run(false)
	stopped <- struct{}{}
}

func appTimeout(app aptima.App, timeout time.Duration) {
	time.Sleep(timeout)
	app.Close()
}

func main() {
	// test app
	app, err := aptima.NewApp(&defaultApp{})
	if err != nil {
		fmt.Println("failed to create app.")
	}

	stopped := make(chan struct{}, 1)
	go appRoutine(app, stopped)
	go appTimeout(app, time.Duration(5*time.Second))
	<-stopped

	fmt.Println("aptima leak obj Size:", aptima.LeakObjSize())
}
