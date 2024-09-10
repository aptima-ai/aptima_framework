//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//

package ten

//#include "data.h"
import "C"

import (
	"unsafe"
)

// Data is the interface for the TEN data message.
type Data interface {
	Msg

	AllocBuf(size int) error
	LockBuf() ([]byte, error)
	UnlockBuf(buf *[]byte) error
	GetBuf() ([]byte, error)
}

type data struct {
	*msg
	size int
}

func newData(bridge C.uintptr_t) *data {
	return &data{msg: newMsg(bridge)}
}

var _ Data = new(data)

func NewData(dataName string) (Data, error) {
	if len(dataName) == 0 {
		return nil, newTenError(
			ErrnoInvalidArgument,
			"data name is required.",
		)
	}

	var bridge C.uintptr_t
	err := withCGO(func() error {
		cStatus := C.ten_go_data_create(
			unsafe.Pointer(unsafe.StringData(dataName)),
			C.int(len(dataName)),
			&bridge,
		)
		e := withGoStatus(&cStatus)

		return e
	})
	if err != nil {
		return nil, err
	}

	if bridge == 0 {
		// Should not happen.
		return nil, newTenError(
			ErrnoInvalidArgument,
			"bridge is nil",
		)
	}

	return newData(bridge), nil
}

func (p *data) AllocBuf(size int) error {
	if size <= 0 {
		return newTenError(ErrnoInvalidArgument, "the size should be > 0")
	}

	err := withCGOLimiter(func() error {
		apiStatus := C.ten_go_data_alloc_buf(p.getCPtr(), C.int(size))
		return withGoStatus(&apiStatus)
	})

	if err == nil {
		p.size = size
	}

	return err
}

func (p *data) LockBuf() ([]byte, error) {
	var bufAddr *C.uint8_t
	var bufSize C.uint64_t

	err := withCGOLimiter(func() error {
		apiStatus := C.ten_go_data_lock_buf(
			p.getCPtr(),
			&bufAddr,
			&bufSize,
		)

		return withGoStatus(&apiStatus)
	})

	if err != nil {
		return nil, err
	}

	// This means the len and cap of this `buf` are equal.
	buf := unsafe.Slice((*byte)(unsafe.Pointer(bufAddr)), int(bufSize))

	return buf, nil
}

func (p *data) UnlockBuf(buf *[]byte) error {
	if buf == nil {
		return newTenError(ErrnoInvalidArgument, "buf is nil")
	}

	err := withCGOLimiter(func() error {
		apiStatus := C.ten_go_data_unlock_buf(
			p.getCPtr(),
			unsafe.Pointer(unsafe.SliceData(*buf)),
		)

		return withGoStatus(&apiStatus)
	})

	if err == nil {
		// The buf can not be used anymore after giving back.
		*buf = nil
	}

	return err
}

func (p *data) GetBuf() ([]byte, error) {
	if p.size == 0 {
		return nil, newTenError(ErrnoInvalidArgument, "call AllocBuf() first")
	}

	buf := make([]byte, p.size)
	err := withCGOLimiter(func() error {
		apiStatus := C.ten_go_data_get_buf(
			p.getCPtr(),
			unsafe.Pointer(&buf[0]),
			C.int(p.size),
		)

		return withGoStatus(&apiStatus)
	})

	if err != nil {
		return nil, err
	}

	return buf, nil
}
