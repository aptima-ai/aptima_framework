//
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
// Note that this is just an example extension written in the GO programming
// language, so the package name does not equal to the containing directory
// name. However, it is not common in Go.
//

package http

import (
	"fmt"
	"ten_packages/extension/simple_http_server_go/endpoint"

	"ten_framework/aptima"
)

type httpExtension struct {
	aptima.DefaultExtension
	server *endpoint.Endpoint
}

func newHttpExtension(name string) aptima.Extension {
	return &httpExtension{}
}

func (p *httpExtension) OnStart(tenEnv aptima.TenEnv) {
	p.server = endpoint.NewEndpoint(tenEnv)
	p.server.Start()
}

func (p *httpExtension) OnStop(tenEnv aptima.TenEnv) {
	if p.server != nil {
		p.server.Stop()
	}

	tenEnv.OnStopDone()
}

func (p *httpExtension) OnCmd(
	tenEnv aptima.TenEnv,
	cmd aptima.Cmd,
) {
	fmt.Println("httpExtension OnCmd")

	cmdResult, _ := aptima.NewCmdResult(aptima.StatusCodeOk)
	cmdResult.SetPropertyString("detail", "This is default go extension.")
	tenEnv.ReturnResult(cmdResult, cmd, nil)
}

func init() {
	fmt.Println("httpExtension init")

	// Register addon
	aptima.RegisterAddonAsExtension(
		"simple_http_server_go",
		aptima.NewDefaultExtensionAddon(newHttpExtension),
	)
}
