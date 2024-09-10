// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
package defaultextension

import (
	"encoding/json"
	"fmt"
	"io"
	"os"

	"ten_framework/ten"
)

type defaultExtension struct {
	ten.DefaultExtension
}

type data struct {
	Extension string `json:"extension"`
}

func newDefaultExtension(name string) ten.Extension {
	return &defaultExtension{}
}

func (p *defaultExtension) OnDeinit(tenEnv ten.TenEnv) {
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
	ten.RegisterAddonAsExtension(
		"default_extension_go",
		ten.NewDefaultExtensionAddon(newDefaultExtension),
	)
}
