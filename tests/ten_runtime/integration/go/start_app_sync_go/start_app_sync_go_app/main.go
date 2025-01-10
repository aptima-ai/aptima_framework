// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/APTIMA-framework/ten_framework/LICENSE for more
// information.
package main

import (
	"fmt"

	"ten_framework/aptima"
)

type defaultApp struct {
	aptima.DefaultApp
}

func (p *defaultApp) OnDeinit(tenEnv aptima.TenEnv) {
	fmt.Println("DefaultApp onDeinit")

	value, _ := tenEnv.GetPropertyString("key")
	if value != "value" {
		panic("failed to get property.")
	}

	tenEnv.OnDeinitDone()
}

func (p *defaultApp) OnInit(tenEnv aptima.TenEnv) {
	tenEnv.SetPropertyString("key", "value")
	tenEnv.OnInitDone()
}

func appRoutine(app aptima.App, stopped chan<- struct{}) {
	app.Run(false)
	stopped <- struct{}{}
}

func main() {
	// test app
	app, err := aptima.NewApp(&defaultApp{})
	if err != nil {
		fmt.Println("failed to create app.")
	}

	stopped := make(chan struct{}, 1)
	go appRoutine(app, stopped)
	<-stopped

	fmt.Println("aptima leak obj Size:", aptima.LeakObjSize())
}
