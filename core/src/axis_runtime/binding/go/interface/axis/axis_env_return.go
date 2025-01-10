//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package aptima

//#include "axis_env.h"
import "C"

func (p *tenEnv) ReturnResult(
	statusCmd CmdResult,
	cmd Cmd,
	handler ErrorHandler,
) error {
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

	cb := goHandleNil
	if handler != nil {
		cb = newGoHandle(handler)
	}

	defer func() {
		p.keepAlive()
		cmd.keepAlive()
		statusCmd.keepAlive()
	}()

	err := withCGO(func() error {
		apiStatus := C.axis_go_axis_env_return_result(
			p.cPtr,
			statusCmd.getCPtr(),
			cmd.getCPtr(),
			cHandle(cb),
		)
		return withCGoError(&apiStatus)
	})

	if err != nil {
		// Clean up the handle if there was an error.
		loadAndDeleteGoHandle(cb)
	}

	return err
}

func (p *tenEnv) ReturnResultDirectly(
	statusCmd CmdResult,
	handler ErrorHandler,
) error {
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

	cb := goHandleNil
	if handler != nil {
		cb = newGoHandle(handler)
	}

	err := withCGO(func() error {
		apiStatus := C.axis_go_axis_env_return_result_directly(
			p.cPtr,
			statusCmd.getCPtr(),
			cHandle(cb),
		)
		return withCGoError(&apiStatus)
	})

	if err != nil {
		// Clean up the handle if there was an error.
		loadAndDeleteGoHandle(cb)
	}

	return err
}
