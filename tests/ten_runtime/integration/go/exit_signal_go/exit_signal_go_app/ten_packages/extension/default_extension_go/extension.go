//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/APTIMA-framework/ten_framework/LICENSE for more
// information.
//

package default_extension_go

import (
	"encoding/json"
	"fmt"
	"io"
	"os"

	"ten_framework/aptima"
)

type defaultExtension struct {
	aptima.DefaultExtension
}

type data struct {
	Extension string `json:"extension"`
}

func newDefaultExtension(name string) aptima.Extension {
	return &defaultExtension{}
}

func (p *defaultExtension) OnDeinit(tenEnv aptima.TenEnv) {
	fmt.Println("defaultExtension OnStop")

	// Write something to file, so the test case can check if this function is
	// always called when app exits.

	d := &data{
		Extension: "exit_signal",
	}

	dest := "exit_signal.json"
	if _, err := os.Stat(dest); err == nil {
		os.Remove(dest)
	}

	dest_file, err := os.Create(dest)
	if err != nil {
		panic(err)
	}

	defer dest_file.Close()

	bytes, _ := json.MarshalIndent(d, "", " ")
	_, err = io.WriteString(dest_file, string(bytes))
	if err != nil {
		panic(err)
	}

	tenEnv.OnDeinitDone()
}

func init() {
	fmt.Println("defaultExtension init")

	// Register addon
	aptima.RegisterAddonAsExtension(
		"default_extension_go",
		aptima.NewDefaultExtensionAddon(newDefaultExtension),
	)
}
