//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//

package ten

import (
	"testing"
)

func TestNewVideoFrame(t *testing.T) {
	frame, err := NewVideoFrame("video_frame")
	if err != nil {
		t.FailNow()
	}

	err = frame.AllocBuf(10)
	if err != nil {
		t.FailNow()
	}

	buf, err := frame.LockBuf()
	if err != nil {
		t.FailNow()
	}

	if len(buf) != 10 {
		t.FailNow()
	}

	buf[0] = 1

	err = frame.UnlockBuf(&buf)
	if err != nil {
		t.FailNow()
	}

	buf2, err := frame.GetBuf()
	if err != nil {
		t.FailNow()
	}

	if buf2[0] != 1 {
		t.FailNow()
	}
}
