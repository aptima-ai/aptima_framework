// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
package standalone

import (
	"testing"

	"ten_framework/ten"
)

func TestDataProperty(t *testing.T) {
	data, err := ten.NewData("test")
	if err != nil {
		t.FailNow()
	}

	if err := data.SetProperty("k", 1); err != nil {
		t.FailNow()
	}

	if v, err := data.GetPropertyInt64("k"); err != nil {
		t.FailNow()
	} else {
		if v != 1 {
			t.FailNow()
		}
	}
}
