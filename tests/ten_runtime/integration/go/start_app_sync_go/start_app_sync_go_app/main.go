// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/TEN-framework/ten_framework/LICENSE for more
// information.
package main

import (
	"fmt"

	"ten_framework/ten"
)

type defaultApp struct {
	ten.DefaultApp
}

func (p *defaultApp) OnDeinit(tenEnv ten.TenEnv) {
	fmt.Println("DefaultApp onDeinit")

	value, _ := tenEnv.GetPropertyString("key")
	if value != "value" {
		panic("failed to get property.")
	}

	tenEnv.OnDeinitDone()
}

func (p *defaultApp) OnInit(tenEnv ten.TenEnv) {
	tenEnv.SetPropertyString("key", "value")
	tenEnv.OnInitDone()
}

func appRoutine(app ten.App, stopped chan<- struct{}) {
	app.Run(false)
	stopped <- struct{}{}
}

func main() {
	// test app
	app, err := ten.NewApp(&defaultApp{})
	if err != nil {
		fmt.Println("failed to create app.")
	}

	stopped := make(chan struct{}, 1)
	go appRoutine(app, stopped)
	<-stopped

	fmt.Println("ten leak obj Size:", ten.LeakObjSize())
}
