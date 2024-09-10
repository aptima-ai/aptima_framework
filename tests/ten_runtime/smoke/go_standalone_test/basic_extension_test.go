// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
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
