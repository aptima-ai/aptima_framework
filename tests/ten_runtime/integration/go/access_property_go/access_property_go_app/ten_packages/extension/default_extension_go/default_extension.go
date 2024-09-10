// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
// Note that this is just an example extension written in the GO programming
// language, so the package name does not equal to the containing directory
// name. However, it is not common in Go.
package defaultextension

import (
	"fmt"

	"ten_framework/ten"
)

type baseExtension struct {
	ten.DefaultExtension
}

func (ext *baseExtension) OnStart(tenEnv ten.TenEnv) {
	fmt.Println("baseExtension OnStart")

	tenEnv.OnStartDone()
}

func (ext *baseExtension) OnStop(tenEnv ten.TenEnv) {
	fmt.Println("baseExtension OnStop")

	tenEnv.OnStopDone()
}

type UserStruct struct {
	Num int
	Str string
}

type NestedUserStruct struct {
	UserData *UserStruct
	StrSlice []string
}

type aExtension struct {
	baseExtension
}

func (p *aExtension) OnCmd(
	tenEnv ten.TenEnv,
	cmd ten.Cmd,
) {
	if err := tenEnv.SetProperty("testBool", false); err != nil {
		panic("Should not happen.")
	}

	if err := tenEnv.SetProperty("testInt", 3); err != nil {
		panic("Should not happen.")
	}

	var testUint uint = 4
	if err := tenEnv.SetProperty("testUint", testUint); err != nil {
		panic("Should not happen.")
	}

	var testUint32 uint32 = 44
	if err := tenEnv.SetProperty("testUint32", testUint32); err != nil {
		panic("Should not happen.")
	}

	var testInt8 int8 = 8
	if err := tenEnv.SetProperty("testInt8", testInt8); err != nil {
		panic("Should not happen.")
	}

	var testInt16 int16 = 16
	if err := tenEnv.SetProperty("testInt16", testInt16); err != nil {
		panic("Should not happen.")
	}

	var testInt32 int32 = 32
	if err := tenEnv.SetProperty("testInt32", testInt32); err != nil {
		panic("Should not happen.")
	}

	var testInt64 int64 = 64
	if err := tenEnv.SetProperty("testInt64", testInt64); err != nil {
		panic("Should not happen.")
	}

	if err := tenEnv.SetProperty("testString", "test string"); err != nil {
		panic("Should not happen.")
	}

	var testFloat32 float32 = 32.0
	if err := tenEnv.SetProperty("testFloat32", testFloat32); err != nil {
		panic("Should not happen.")
	}

	var testFloat64 float64 = 64.0
	if err := tenEnv.SetProperty("testFloat64", testFloat64); err != nil {
		panic("Should not happen.")
	}

	if err := tenEnv.SetProperty(
		"testObject",
		&UserStruct{2, "hello"},
	); err != nil {
		panic("Should not happen.")
	}

	// The struct property is not supported.
	if err := tenEnv.SetProperty(
		"testStruct",
		NestedUserStruct{
			UserData: &UserStruct{5, "world"},
			StrSlice: []string{"a", "b", "c"},
		},
	); err == nil {
		panic("Should not happen.")
	}

	strBytes := []byte("hello")
	if err := tenEnv.SetProperty("testByteArray", strBytes); err != nil {
		panic("Should not happen.")
	}

	// ================================================

	testBool, err := tenEnv.GetPropertyBool("testBool")
	if err != nil || testBool != false {
		panic("Should not happen.")
	}

	testInt, err := tenEnv.GetPropertyInt64("testInt")
	if err != nil || testInt != 3 {
		panic("Should not happen.")
	}

	testUint64, err := tenEnv.GetPropertyUint64("testUint")
	if err != nil || testUint64 != 4 {
		panic("Should not happen.")
	}

	testUint32, err = tenEnv.GetPropertyUint32("testUint32")
	if err != nil || testUint32 != 44 {
		panic("Should not happen.")
	}

	testInt8, err = tenEnv.GetPropertyInt8("testInt8")
	if err != nil || testInt8 != 8 {
		panic("Should not happen.")
	}

	testInt16, err = tenEnv.GetPropertyInt16("testInt16")
	if err != nil || testInt16 != 16 {
		panic("Should not happen.")
	}

	testInt32, err = tenEnv.GetPropertyInt32("testInt32")
	if err != nil || testInt32 != 32 {
		panic("Should not happen.")
	}

	testInt64, err = tenEnv.GetPropertyInt64("testInt64")
	if err != nil || testInt64 != 64 {
		panic("Should not happen.")
	}

	testFloat32, err = tenEnv.GetPropertyFloat32("testFloat32")
	if err != nil || testFloat32 != 32.0 {
		panic("Should not happen.")
	}

	testFloat64, err = tenEnv.GetPropertyFloat64("testFloat64")
	if err != nil || testFloat64 != 64.0 {
		panic("Should not happen.")
	}

	testString, err := tenEnv.GetPropertyString("testString")
	if err != nil || testString != "test string" {
		panic("Should not happen.")
	}

	testObject, err := tenEnv.GetPropertyPtr(
		"testObject",
	)
	if err != nil || testObject.(*UserStruct).Num != 2 {
		panic("Should not happen.")
	}

	testByteArray, err := tenEnv.GetPropertyBytes(
		"testByteArray",
	)
	if err != nil || string(testByteArray) != "hello" {
		panic("Should not happen.")
	}

	cmdResult, _ := ten.NewCmdResult(ten.StatusCodeOk)
	cmdResult.SetPropertyString("detail", "okok")
	err = tenEnv.ReturnResult(cmdResult, cmd)
	if err != nil {
		panic("ReturnResult failed")
	}
}

type nodeTestGroup struct {
	ten.DefaultExtensionGroup
}

func newNodeTestGroup(name string) ten.ExtensionGroup {
	return &nodeTestGroup{}
}

func (p *nodeTestGroup) OnCreateExtensions(
	tenEnv ten.TenEnv,
) {
	tenEnv.OnCreateExtensionsDone(
		ten.WrapExtension(&aExtension{}, "A"),
	)
}

func init() {
	// Register addon
	ten.RegisterAddonAsExtensionGroup(
		"nodetest",
		ten.NewDefaultExtensionGroupAddon(newNodeTestGroup),
	)
}
