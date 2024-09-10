//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//

package ten

//#include "ten.h"
import "C"

func (p *tenEnv) ReturnResult(statusCmd CmdResult, cmd Cmd) error {
	if statusCmd == nil {
		return newTenError(
			ErrnoInvalidArgument,
			"cmd result is required.",
		)
	}

	if cmd == nil {
		return newTenError(
			ErrnoInvalidArgument,
			"cmd is required.",
		)
	}

	defer func() {
		p.keepAlive()
		cmd.keepAlive()
		statusCmd.keepAlive()
	}()

	return withCGO(func() error {
		apiStatus := C.ten_go_ten_env_return_result(
			p.cPtr,
			statusCmd.getCPtr(),
			cmd.getCPtr(),
		)
		return withGoStatus(&apiStatus)
	})
}

func (p *tenEnv) ReturnResultDirectly(statusCmd CmdResult) error {
	if statusCmd == nil {
		return newTenError(
			ErrnoInvalidArgument,
			"cmd result is required.",
		)
	}

	defer func() {
		p.keepAlive()
		statusCmd.keepAlive()
	}()

	return withCGO(func() error {
		apiStatus := C.ten_go_ten_env_return_result_directly(
			p.cPtr,
			statusCmd.getCPtr(),
		)
		return withGoStatus(&apiStatus)
	})
}
