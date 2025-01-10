//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import * as path from "path";
import * as fs from "fs";

import { Addon } from "./addon";
import axis_addon from "../axis_addon";

type Ctor<T> = {
  new(): T;
  prototype: T;
};

type addonRegisterHandler = (registerContext: any) => void;

export class AddonManager {
  private static _registry: Map<string, addonRegisterHandler> = new Map();

  static _set_register_handler(
    name: string,
    handler: addonRegisterHandler
  ): void {
    AddonManager._registry.set(name, handler);
  }

  static _find_app_base_dir(): string {
    let currentDir = __dirname;

    while (currentDir !== path.dirname(currentDir)) {
      const manifestPath = path.join(currentDir, "manifest.json");
      if (fs.existsSync(manifestPath)) {
        try {
          const manifestJson = JSON.parse(
            fs.readFileSync(manifestPath, "utf-8")
          );
          if (manifestJson.type === "app") {
            return currentDir;
          }
        } catch (error) {
          // Ignore
        }
      }

      currentDir = path.dirname(currentDir);
    }

    throw new Error("Cannot find app base dir");
  }

  static _load_all_addons(): void {
    const app_base_dir = AddonManager._find_app_base_dir();

    const manifest_path = path.join(app_base_dir, "manifest.json");
    if (!fs.existsSync(manifest_path)) {
      throw new Error("Cannot find manifest.json");
    }

    const manifest = JSON.parse(fs.readFileSync(manifest_path, "utf-8"));

    let extension_names = [];

    const dependencies = manifest.dependencies;
    for (let dep of dependencies) {
      if (dep.type === "extension") {
        extension_names.push(dep.name);
      }
    }

    const extension_folder = path.join(app_base_dir, "axis_packages/extension");
    if (!fs.existsSync(extension_folder)) {
      return;
    }

    const dirs = fs.opendirSync(extension_folder);
    for (; ;) {
      const entry = dirs.readSync();
      if (!entry) {
        break;
      }

      if (entry.name.startsWith(".")) {
        continue;
      }

      const packageJsonFile = `${extension_folder}/${entry.name}/package.json`;

      if (entry.isDirectory() && fs.existsSync(packageJsonFile)) {
        require(`${extension_folder}/${entry.name}`);
      }
    }
  }

  static _register_all_addons(registerContext: any): void {
    for (let [name, handler] of AddonManager._registry) {
      try {
        handler(registerContext);

        console.log(`Addon ${name} registered`);
      } catch (error) {
        console.error(`Failed to register addon ${name}: ${error}`);
      }
    }

    AddonManager._registry.clear();
  }
}

export function RegisterAddonAsExtension(
  name: string
): <T extends Ctor<Addon>>(klass: T) => T {
  return function <T extends Ctor<Addon>>(klass: T): T {
    function registerHandler(registerContext: any) {
      const addon_instance = new klass();

      axis_addon.axis_nodejs_addon_manager_register_addon_as_extension(
        name,
        addon_instance
      );
    }

    AddonManager._set_register_handler(name, registerHandler);

    return klass;
  };
}
