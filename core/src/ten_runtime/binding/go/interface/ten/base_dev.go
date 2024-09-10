//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//

//go:build dev

package ten

import "fmt"

// All methods in this file are only enabled with build flags: -tags "dev".

// escapeCheck checks if the baseTenObject is invalid now, ex: after calling
// SendCmd.
//
// In "dev" mode, we will check if the calling sequences on one baseTenObject
// are unexpected. This method will panic if the baseTenObject is invalid.
func (p *baseTenObject[T]) escapeCheck(fn func()) {
	defer p.RUnlock()
	p.RLock()

	if p.cPtr == nil {
		panic(fmt.Sprintf("Calling on an object %+v which is out of scope.", p))
	}

	fn()
}
