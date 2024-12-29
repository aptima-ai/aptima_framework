//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package ten

import (
	"errors"
	"fmt"
	"log"
	"sync"
	"sync/atomic"
)

// Summary:
//
// 1. Executor Creation and Start:
//   - Executors are created with a specified capacity and optional warm-up and
//     panic handling functions.
//   - Starting an executor involves running its warm-up function (if provided)
//     and then continuously processing tasks from its queue.
//
// 2. Task Processing:
//   - Tasks are executed in a FIFO order.
//   - A recovery mechanism is in place to handle panics during task execution.
//
// 3. Executor Pool Management:
//   - The executor pool manages multiple executors, distributing tasks among
//     them using the load balancer.
//   - It provides functionality to submit tasks to the pool and handle the
//     pool's lifecycle (including graceful shutdown).
//
// 4. Error Handling and Logging:
//   - Custom errors are used to signal various conditions (e.g., invalid tasks,
//     pool closure).
//   - The default panic handler logs errors using a specific logging strategy.

type (
	taskFunc     func()
	warmupFunc   func() error
	onClosedHook func()
	onPanicFunc  func(any)
)

const (
	// Default values for the number of workers.
	defaultWorkerSize = 1

	// Default values for the task queue capacity.
	defaultTaskQueueCap = 1
)

var (
	// Predefined error values for specific error scenarios.
	errInvalidTask         = errors.New("task is required.")
	errPoolClosed          = errors.New("the pool has been closed")
	errNoAvailableExecutor = errors.New("no available executor")

	// defaultOnPanic is the default function to handle panics.
	defaultOnPanic = func(p any) {
		var m string

		// Determine the type of the p variable.
		switch pt := p.(type) {
		case string:
			m = pt
		case error:
			m = pt.Error()
		case fmt.Stringer:
			m = pt.String()
		default:
			m = fmt.Sprintf("%+v", p)
		}

		// 'fmt.Println' is not used because its output cannot be redirected to
		// other loggers. Meanwhile, the `slog` can not be used here as it is
		// available since GO 1.21.
		log.Printf("Panic recovered: %s.\n", m)
	}
)

// executor represents a single task executor with a queue, warm-up function,
// panic handler, and a shutdown mechanism.
type executor struct {
	// This channel is used to queue tasks to be executed by the executor.
	taskQueue chan taskFunc

	warmup  warmupFunc
	onPanic onPanicFunc

	// Used to signal the shutdown of the executor, allowing for controlled
	// termination.
	shutdown chan struct{}
	closing  atomic.Bool

	// A onClosedHook.
	onClosed atomic.Value
}

// newExecutor constructor of executor.
// - cap: Specify the capacity of the task queue.
// - wf: A function that is called during the startup of the executor.
// - op: A function to be called in case of a panic during task execution.
func newExecutor(cap int, wf warmupFunc, op onPanicFunc) *executor {
	if cap <= 0 {
		cap = defaultTaskQueueCap
	}

	return &executor{
		taskQueue: make(chan taskFunc, cap),
		warmup:    wf,
		onPanic:   op,
		shutdown:  make(chan struct{}, 1),
	}
}

func (exec *executor) runUntilClosed() {
	running := true

	// Continuously process tasks.
	for running {
		select {
		// Shutdown immediately.
		case <-exec.shutdown:
			running = false

		case task := <-exec.taskQueue:
			// Graceful shutdown, ensure that all tasks would be executed.
			if task == nil {
				running = false
				break
			}

			func() {
				// Using a closure to execute the task, then the panic could be
				// recovered and the [executor] can continue.
				defer func() {
					if exec.onPanic != nil {
						if p := recover(); p != nil {
							exec.onPanic(p)
						}
					}
				}()

				task()
			}()
		}
	}
}

// start the executor. Initiates the starting process of the executor by
// executing potentially required warm-up procedures and then continuously
// processing tasks until a shutdown signal is received.
func (exec *executor) start() error {
	// This channel is used to signal the completion of the warm-up process.
	ready := make(chan error, 1)

	go func() {
		var err error

		if exec.warmup != nil {
			err = exec.warmup()
		}

		ready <- err
		if err != nil {
			return
		}

		exec.runUntilClosed()

		// The goroutine is closed normally.
		hook := exec.onClosed.Load()
		if hook != nil {
			hook.(onClosedHook)()
		}
	}()

	err := <-ready
	return err
}

func (exec *executor) stop(graceful bool, hook onClosedHook) {
	if !exec.closing.CompareAndSwap(false, true) {
		return
	}

	exec.onClosed.Store(hook)

	if graceful {
		exec.taskQueue <- nil
	} else {
		exec.shutdown <- struct{}{}
	}
}

func (exec *executor) submit(task taskFunc) bool {
	if exec.closing.Load() {
		return false
	}

	exec.taskQueue <- task
	return true
}

// For distributing tasks among executors.
type iLoadBalance interface {
	route(taskFunc) int
}

// roundRobin uses a simple round-robin algorithm, with an optimization for
// power-of-two worker sizes.
type roundRobin struct {
	workerSize    uint64
	currentRouter atomic.Uint64
	preferBitOp   bool
	bitOp         uint64
}

func newRoundRobin(size int) *roundRobin {
	if size <= 1 {
		// No need to use a load balance.
		return nil
	}

	return &roundRobin{
		workerSize:  uint64(size),
		preferBitOp: size&(size-1) == 0,
		bitOp:       uint64(size - 1),
	}
}

// route is used to determine which executor or worker should handle a given
// task.
func (rr *roundRobin) route(tf taskFunc) int {
	// If the currentRouter is overflow after Add, it will start from 0 again.
	size := rr.currentRouter.Add(1)

	if rr.preferBitOp {
		return int(size & rr.bitOp)
	}
	return int(size % rr.workerSize)
}

// executorPoolConfig helps to initialize the executorPool. It defines the
// configuration for an executor pool, including worker size, queue size,
// warm-up function, and panic handler.
//
//   - workerSize the number of workers running in the executorPool. We only
//     support a fixed size now. Default is 1. Each worker will start a
//     goroutine. The workerSize is preferred to be powers of 2 if you want to
//     set it > 1.
//
//   - waitingListSize the size of task queue of each executor. If the queue is
//     full, calling [submit] will be blocked. Default is 1.
//
//   - warmup is used to do some initialization in the executor's goroutine. Ex:
//     if you want the executor to be run on the same OS thread, calling
//     runtime.LockOSThread() in the warmup.
//
// - onPanic recovers the panic which is caused by the tasks.
type executorPoolConfig struct {
	workerSize      int
	waitingListSize int
	warmup          warmupFunc
	onPanic         onPanicFunc
}

// A pool of executors with load balancing.
type executorPool struct {
	cap       int
	closing   atomic.Bool
	executors []*executor
	lb        iLoadBalance
}

func (cfg *executorPoolConfig) withDefault() {
	if cfg.workerSize <= 0 {
		cfg.workerSize = defaultWorkerSize
	}

	if cfg.onPanic == nil {
		cfg.onPanic = defaultOnPanic
	}
}

// newExecutorPool creates a goroutine pool. Refer executorPoolConfig to see the
// configuration. The returned executorPool will be nil if the [warmup] function
// fails, and the corresponding error will be returned.
//
// Note that we do not provide a start method on executorPool, all the executors
// in it will be started after this function returns.
func newExecutorPool(config executorPoolConfig) (*executorPool, error) {
	var err error

	config.withDefault()
	p := &executorPool{
		cap:       config.workerSize,
		executors: make([]*executor, 0, config.workerSize),
		lb:        newRoundRobin(config.workerSize),
	}

	// Create individual executors.
	for i := 0; i < config.workerSize; i++ {
		ex := newExecutor(config.waitingListSize, config.warmup, config.onPanic)

		// Starts the executor's operation, including running any warm-up logic.
		err = ex.start()
		if err != nil {
			break
		}

		p.executors = append(p.executors, ex)
	}

	if err != nil {
		// If there was an error starting any of the executors, the function
		// returns nil for the pool and the encountered error.
		p.release(false)
		return nil, err
	}

	return p, nil
}

// release the executorPool and wait it to be released completely. If [graceful]
// is true, all the tasks in the waiting list would be executed. Otherwise, the
// executors in the pool will be shutdown as soon as possible.
func (p *executorPool) release(graceful bool) {
	// First checks if the pool is already in the process of closing using an
	// atomic boolean (exec.closing). If the pool is already closing, the method
	// returns early. This check prevents redundant shutdown attempts.
	if !p.closing.CompareAndSwap(false, true) {
		return
	}

	size := len(p.executors)
	if size == 0 {
		return
	}

	var allStopped sync.WaitGroup
	allStopped.Add(size)

	hook := func() {
		allStopped.Done()
	}

	// For each executor, it calls the executor's stop method, passing the
	// graceful flag and the hook. This instructs each executor to shut down,
	// either immediately or after completing their current task, depending on
	// the value of graceful.
	for i := 0; i < size; i++ {
		p.executors[i].stop(graceful, hook)
	}

	// Ensures that the release method only returns after all executors have
	// fully stopped.
	allStopped.Wait()
}

// submit a task to the pool, and pick an executor to execute it.
// Returns errInvalidTask if [task] is nil.
// Returns errPoolClosed if release has been called.
// Returns errNoAvailableExecutor if the selected executor has been closed.
//
// Note that the task is always executed async (i.e., in another goroutine which
// is different with the caller), it's your responsibility to send the result to
// your goroutine if needed, ex: using channel. Please keep in mind that, do not
// block the executor. Ex: if you want to use channel to send the result, the
// channel must have enough buffer before submitting the task to the executor,
// in other words, the [chansend] operator can not be blocked.
func (p *executorPool) submit(task taskFunc) error {
	if task == nil {
		return errInvalidTask
	}

	if p.closing.Load() {
		return errPoolClosed
	}

	idx := 0
	if p.cap > 1 {
		idx = p.lb.route(task)
	}

	success := p.executors[idx].submit(task)
	if !success {
		return errNoAvailableExecutor
	}

	return nil
}
