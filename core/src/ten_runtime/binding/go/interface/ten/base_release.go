//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//

//go:build !dev

package ten

// All methods in this file are only enabled without build flags: -tags "dev".

// escapeCheck checks if the baseTenObject is invalid now, ex: after calling
// SendCmd.
//
// There's no check in "release" mode, it's the user's responsibility to make
// sure the calling sequences on one baseTenObject are correct.
func (p *baseTenObject[T]) escapeCheck(fn func()) {
	fn()
}
