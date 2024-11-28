//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package ten

//#include "ten_env.h"
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
		return withCGoError(&apiStatus)
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
		return withCGoError(&apiStatus)
	})
}
