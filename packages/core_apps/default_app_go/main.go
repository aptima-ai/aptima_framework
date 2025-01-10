//
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//

package main

// To detect memory leaks with ASan, need to enable the following cgo code.

// void __lsan_do_leak_check(void);
// import "C"

import (
	"fmt"

	"axis_framework/aptima"
)

type defaultApp struct {
	aptima.DefaultApp
}

func (p *defaultApp) OnInit(tenEnv aptima.TenEnv) {
	tenEnv.LogDebug("onInit")
	tenEnv.OnInitDone()
}

func (p *defaultApp) OnDeinit(tenEnv aptima.TenEnv) {
	tenEnv.LogDebug("onDeinit")
	tenEnv.OnDeinitDone()
}

func main() {
	// test app
	app, err := aptima.NewApp(&defaultApp{})
	if err != nil {
		fmt.Println("Failed to create app.")
	}

	app.Run(true)
	app.Wait()

	// To detect memory leaks with ASan, need to enable the following cgo code.
	// runtime.GC()
	// C.__lsan_do_leak_check()
}
