//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import {
  Addon,
  RegisterAddonAsExtension,
  Extension,
  TenEnv,
  Cmd,
} from "ten-runtime-nodejs";

function assert(condition: boolean, message: string) {
  if (!condition) {
    throw new Error(message);
  }
}

class DefaultExtension extends Extension {
  constructor(name: string) {
    super(name);
  }

  async onConfigure(_tenEnv: TenEnv): Promise<void> {
    console.log("DefaultExtension onConfigure");
  }

  async onInit(tenEnv: TenEnv): Promise<void> {
    console.log("DefaultExtension onInit");

    const aaaExist = await tenEnv.isPropertyExist("aaa");
    const bbbExist = await tenEnv.isPropertyExist("bbb");
    assert(aaaExist, "aaa not exist");
    assert(!bbbExist, "bbb exist");

    const intValue = await tenEnv.getPropertyNumber("keyInt");
    assert(intValue === -32141, "intValue incorrect");

    const floatValue = await tenEnv.getPropertyNumber("keyFloat");
    assert(floatValue > 3.14 && floatValue < 3.15, "floatValue incorrect");

    let exceptionCaught = false;
    try {
      const _ = await tenEnv.getPropertyNumber("nonExistNumKey");
    } catch (e) {
      console.error("Caught expected exception for nonExistNumKey:", e);
      exceptionCaught = true;
    }
    assert(exceptionCaught, "exception not caught");

    const stringValue = await tenEnv.getPropertyString("keyString");
    assert(stringValue === "hello", "stringValue incorrect");

    exceptionCaught = false;
    try {
      const _ = await tenEnv.getPropertyString("nonExistStringKey");
    } catch (e) {
      console.error("Caught expected exception for nonExistStringKey:", e);
      exceptionCaught = true;
    }
    assert(exceptionCaught, "exception not caught");

    const propertyJsonStr = await tenEnv.getPropertyToJson("keyObject");
    const propertyJson = JSON.parse(propertyJsonStr);
    assert(propertyJson.key1 === "value1", "propertyJson incorrect");
    assert(propertyJson.key2 === 2, "propertyJson incorrect");

    exceptionCaught = false;
    try {
      const _ = await tenEnv.getPropertyToJson("nonExistObjectKey");
    } catch (e) {
      console.error("Caught expected exception for nonExistObjectKey:", e);
      exceptionCaught = true;
    }
    assert(exceptionCaught, "exception not caught");

    await tenEnv.setPropertyNumber("setKeyInt", 12345);
    await tenEnv.setPropertyNumber("setKeyFloat", 3.1415);
    await tenEnv.setPropertyString("setKeyString", "happy");
    await tenEnv.setPropertyFromJson(
      "setKeyObject",
      JSON.stringify({ key1: "value1", key2: 2 })
    );

    const setIntValue = await tenEnv.getPropertyNumber("setKeyInt");
    assert(setIntValue === 12345, "setIntValue incorrect");

    const setFloatValue = await tenEnv.getPropertyNumber("setKeyFloat");
    assert(
      setFloatValue > 3.1414 && setFloatValue < 3.1416,
      "setFloatValue incorrect"
    );

    const setStringValue = await tenEnv.getPropertyString("setKeyString");
    assert(setStringValue === "happy", "setStringValue incorrect");

    const setPropertyJsonStr = await tenEnv.getPropertyToJson("setKeyObject");
    const setPropertyJson = JSON.parse(setPropertyJsonStr);
    assert(setPropertyJson.key1 === "value1", "setPropertyJson incorrect");
    assert(setPropertyJson.key2 === 2, "setPropertyJson incorrect");

    const cmd = Cmd.Create("hello");
    const cmdName = cmd.getName();
    assert(cmdName === "hello", "cmdName incorrect");
  }

  async onStart(_tenEnv: TenEnv): Promise<void> {
    console.log("DefaultExtension onStart");
  }

  async onStop(_tenEnv: TenEnv): Promise<void> {
    console.log("DefaultExtension onStop");
  }

  async onDeinit(_tenEnv: TenEnv): Promise<void> {
    console.log("DefaultExtension onDeinit");
  }
}

@RegisterAddonAsExtension("default_extension_nodejs")
class DefaultExtensionAddon extends Addon {
  async onInit(_tenEnv: TenEnv): Promise<void> {
    console.log("DefaultExtensionAddon onInit");
  }

  async onCreateInstance(
    _tenEnv: TenEnv,
    instanceName: string
  ): Promise<Extension> {
    return new DefaultExtension(instanceName);
  }

  async onDeinit(_tenEnv: TenEnv): Promise<void> {
    console.log("DefaultExtensionAddon onDeinit");
  }
}
