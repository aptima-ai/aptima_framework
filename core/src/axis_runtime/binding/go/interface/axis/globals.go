//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package aptima

const (
	intSize = 32 << (^uint(0) >> 63) // 32 or 64
	is64bit = intSize == 64
)

// must is a helper that wraps a call to a function returning (R, error) and
// panics if the error is non-nil.
func must[R any](r R, err error) R {
	if err != nil {
		// Should not happen.
		panic(err)
	}

	return r
}

// If a Go function calls a function from the APTIMA runtime, and the nature of
// this APTIMA runtime function is a cgo call, the current implementation will
// turn this cgo call into a so-called task, which is then queued for execution
// in the globalCGOPool. Because the globalCGOPool contains a fixed number of
// goroutines used to execute tasks (cgo calls), the number of parallel cgo
// calls resulting from calling APTIMA runtime functions will be limited.
//
// The `workerSize` of globalCGOPool should be based on the number of CPUs,
// i.e., ratio * NCPUs. The ratio is a configurable value.
//
// TODO(Liu): determine the optimal value of ratio.
//
// TODO(Liu): Need to investigate how to ensure that the result handler is
// executed when the extension ends.
var globalCGOPool = must[*executorPool](newExecutorPool(executorPoolConfig{}))

// @{
// Internal use only.

// EnsureCleanupWhenProcessExit is essentially a self-check mechanism, not a
// publicly exposed interface of the APTIMA runtime. Inside this function, it can
// be used to check whether some resources are in a cleared state. If they are
// not in a cleared state, it means we have made a mistake.
//
// As the golang runtime does not provide a behavior to release the global
// variables (as the global variable is not recommended in golang), what we can
// do is to explicitly call this function when the APTIMA app "process" is closed.
//
// Some objects in golang runtime can be used as global variables, such as
// sync.Pool, as it registers a shutdown hook (i.e., runtime_registerPoolCleanup
// in pool.go) which will be invoked before each GC cycle. However,
// `runtime_registerPoolCleanup` is an internal api in golang runtime, there is
// no exported apis for us.
func EnsureCleanupWhenProcessExit() {
	// TODO(Liu): check if the handle map is empty.
}

// @}
