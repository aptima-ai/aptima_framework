//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package ten

import (
	"runtime"
	"sync"
	"testing"
)

func TestExecutorPoolOnTaskPanic(t *testing.T) {
	p, err := newExecutorPool(executorPoolConfig{
		workerSize: 8,
	})
	if err != nil {
		t.FailNow()
	}

	defer p.release(true)

	var wg sync.WaitGroup
	wg.Add(2)

	err = p.submit(func() {
		wg.Done()
		panic("Pan...")
	})
	if err != nil {
		t.FailNow()
	}

	_ = p.submit(func() {
		wg.Done()
		panic("Panic again...")
	})

	wg.Wait()
}

func TestExecutorPoolShutdown(t *testing.T) {
	p, err := newExecutorPool(executorPoolConfig{})
	if err != nil {
		t.FailNow()
	}

	defer p.release(false)

	for i := 0; i < 10000; i++ {
		err = p.submit(func() {})
		if err != nil {
			t.FailNow()
		}
	}
}

// One worker, and waiting list is 1.
//
// The benchmark is:
// goos: linux
// goarch: amd64
// cpu: Intel(R) Core(TM) i7-10510U CPU @ 1.80GHz
//
// 4663488	 249.4 ns/op	   16 B/op    1 allocs/op
func BenchmarkExecutorPoolOneOne(b *testing.B) {
	p, err := newExecutorPool(executorPoolConfig{})
	if err != nil {
		b.FailNow()
	}

	defer p.release(true)

	var wg sync.WaitGroup
	wg.Add(b.N)

	b.ResetTimer()

	for i := 0; i < b.N; i++ {
		err = p.submit(func() {
			wg.Done()
		})

		if err != nil {
			b.FailNow()
		}
	}

	b.ReportAllocs()

	wg.Wait()
}

// Ten workers, and waiting list of each worker is 1.
//
// The benchmark is:
// goos: linux
// goarch: amd64
// cpu: Intel(R) Core(TM) i7-10510U CPU @ 1.80GHz
// 3864484	  276.1 ns/op	  16 B/op    1 allocs/op
func BenchmarkExecutorPoolTenOne(b *testing.B) {
	p, err := newExecutorPool(executorPoolConfig{
		workerSize: 8,
	})
	if err != nil {
		b.FailNow()
	}

	defer p.release(true)

	var wg sync.WaitGroup
	wg.Add(b.N)

	b.ResetTimer()

	for i := 0; i < b.N; i++ {
		err = p.submit(func() {
			wg.Done()
		})

		if err != nil {
			b.FailNow()
		}
	}

	b.ReportAllocs()

	wg.Wait()
}

// One worker and waiting list is 10.
//
// The benchmark is:
// goos: linux
// goarch: amd64
// cpu: Intel(R) Core(TM) i7-10510U CPU @ 1.80GHz
// 6495974	       156.5 ns/op	      16 B/op	       1 allocs/op
func BenchmarkExecutorPoolOneTen(b *testing.B) {
	p, err := newExecutorPool(executorPoolConfig{
		waitingListSize: 10,
	})
	if err != nil {
		b.FailNow()
	}

	defer p.release(true)

	var wg sync.WaitGroup
	wg.Add(b.N)

	for i := 0; i < b.N; i++ {
		err = p.submit(func() {
			wg.Done()
		})

		if err != nil {
			b.FailNow()
		}
	}

	b.ReportAllocs()

	wg.Wait()
}

// One worker, and waiting list is 1, and the worker always runs on the same OS
// thread.
//
// The benchmark is:
// goos: linux
// goarch: amd64
// cpu: Intel(R) Core(TM) i7-10510U CPU @ 1.80GHz
// 522252	      1994 ns/op	      16 B/op	       1 allocs/op
func BenchmarkExecutorPoolLockOSThread(b *testing.B) {
	p, err := newExecutorPool(executorPoolConfig{
		warmup: func() error {
			runtime.LockOSThread()
			return nil
		},
	})
	if err != nil {
		b.FailNow()
	}

	defer p.release(true)

	var wg sync.WaitGroup
	wg.Add(b.N)

	for i := 0; i < b.N; i++ {
		err = p.submit(func() {
			wg.Done()
		})

		if err != nil {
			b.FailNow()
		}
	}

	b.ReportAllocs()

	wg.Wait()
}
