// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to https://github.com/TEN-framework/ten_framework/LICENSE for more
// information.
package standalone

import (
	"testing"

	"ten_framework/ten"
)

type tenEnvMock struct {
	ten.TenEnv
	t *testing.T
}

func (m *tenEnvMock) ReturnResult(
	statusCmd ten.CmdResult,
	cmd ten.Cmd,
) error {
	detail, err := statusCmd.GetPropertyString("detail")
	if err != nil {
		m.t.Errorf("GetPropertyString failed: %v", err)
		m.t.Fail()
	}

	if detail != "success" {
		m.t.Errorf("Expect: success, actual: %s", detail)
		m.t.Fail()
	}

	return nil
}

func TestBasicExtensionOnCmd(t *testing.T) {
	ext := newBasicExtension("test")
	tenEnv := &tenEnvMock{t: t}

	cmd, err := ten.NewCmd("test")
	if err != nil {
		t.Fail()
	}

	ext.OnCmd(tenEnv, cmd)
}
