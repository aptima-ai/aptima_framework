//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package ten

import (
	"encoding/json"
	"fmt"
	"runtime/pprof"
	"testing"
)

// * Environment:
//   - LD_LIBRARY_PATH: <TEN_PLATFORM>/out/linux/x64
//   - CGO_LDFLAGS: -L<TEN_PLATFORM>/out/linux/x64 -lten_runtime_go
//     -Wl,-rpath,@loader_path/lib -Wl,-rpath,@loader_path/../lib
//
// * Test Kind: Package
func TestNewCustomCmd(t *testing.T) {
	c, err := NewCmd("test")
	if err != nil {
		t.FailNow()
	}

	cn, _ := c.GetName()
	if cn != "test" {
		t.FailNow()
	}
}

// The benchmark is:
//
// goos: linux
// goarch: amd64
// cpu: Intel(R) Core(TM) i7-10510U CPU @ 1.80GHz
//
// -8  308562	      3245 ns/op	     112 B/op	       4 allocs/op
func BenchmarkNewCustomCmd(b *testing.B) {
	threads := pprof.Lookup("threadcreate")
	fmt.Printf("Before testing, thread count: %d\n", threads.Count())

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		c, err := NewCmd("test")
		if err != nil {
			b.FailNow()
		}

		if c == nil {
			b.FailNow()
		}
	}

	fmt.Printf("After testing, thread count: %d\n", threads.Count())

	b.ReportAllocs()
}

// The benchmark is:
//
// goos: linux
// goarch: amd64
// cpu: Intel(R) Core(TM) i7-10510U CPU @ 1.80GHz
//
// -8  451039	      2400 ns/op	     112 B/op	       4 allocs/op
func BenchmarkNewCustomCmd2(b *testing.B) {
	threads := pprof.Lookup("threadcreate")
	fmt.Printf("Before testing, thread count: %d\n", threads.Count())

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		for pb.Next() {
			c, err := NewCmd("test")
			if err != nil {
				b.FailNow()
			}

			if c == nil {
				b.FailNow()
			}
		}
	})

	fmt.Printf("After testing, thread count: %d\n", threads.Count())

	b.ReportAllocs()
}

func TestNewCustomCmdFromJsonBytes(t *testing.T) {
	cmdHeader := make(map[string]any)
	cmdSection := make(map[string]string)
	cmdSection["name"] = "test"
	cmdHeader["_ten"] = cmdSection

	jsonBytes, _ := json.Marshal(cmdHeader)

	c, err := NewCmdFromJSONBytes(jsonBytes)
	if err != nil {
		t.FailNow()
	}

	if c == nil {
		t.FailNow()
	}

	cmdName, _ := c.GetName()
	if cmdName != "test" {
		t.FailNow()
	}
}

// The benchmark is:
//
// goos: linux
// goarch: amd64
// cpu: Intel(R) Core(TM) i7-10510U CPU @ 1.80GHz
//
// -8  83959	     12079 ns/op	     144 B/op	       6 allocs/op
func BenchmarkNewCustomCmdFromJsonBytes(b *testing.B) {
	cmdHeader := make(map[string]any)
	cmdSection := make(map[string]string)
	cmdSection["name"] = "test"
	cmdHeader["_ten"] = cmdSection

	jsonBytes, _ := json.Marshal(cmdHeader)

	threads := pprof.Lookup("threadcreate")
	fmt.Printf("Before testing, thread count: %d\n", threads.Count())

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		c, err := NewCmdFromJSONBytes(jsonBytes)
		if err != nil {
			b.FailNow()
		}

		if c == nil {
			b.FailNow()
		}
	}

	fmt.Printf("After testing, thread count: %d\n", threads.Count())

	b.ReportAllocs()
}

// The benchmark is:
//
// goos: linux
// goarch: amd64
// cpu: Intel(R) Core(TM) i7-10510U CPU @ 1.80GHz
//
// -8  106957	     11429 ns/op	     144 B/op	       6 allocs/op
func BenchmarkNewCustomCmdFromJsonBytes2(b *testing.B) {
	cmdHeader := make(map[string]any)
	cmdSection := make(map[string]string)
	cmdSection["name"] = "test"
	cmdHeader["_ten"] = cmdSection

	jsonBytes, _ := json.Marshal(cmdHeader)

	threads := pprof.Lookup("threadcreate")
	fmt.Printf("Before testing, thread count: %d\n", threads.Count())

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		for pb.Next() {
			c, err := NewCmdFromJSONBytes(jsonBytes)
			if err != nil {
				b.FailNow()
			}

			if c == nil {
				b.FailNow()
			}
		}
	})

	fmt.Printf("After testing, thread count: %d\n", threads.Count())

	b.ReportAllocs()
}
