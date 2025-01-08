//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import { AddonManager } from "../addon/addon_manager";
import axis_addon from "../axis_addon";
import { TenEnv } from "../axis_env/axis_env";

export class App {
  constructor() {
    axis_addon.axis_nodejs_app_create(this);
  }

  private async onConfigureProxy(tenEnv: TenEnv): Promise<void> {
    await this.onConfigure(tenEnv);

    AddonManager._load_all_addons();
    AddonManager._register_all_addons(null);

    axis_addon.axis_nodejs_axis_env_on_configure_done(tenEnv);
  }

  private async onInitProxy(tenEnv: TenEnv): Promise<void> {
    await this.onInit(tenEnv);

    axis_addon.axis_nodejs_axis_env_on_init_done(tenEnv);
  }

  private async onDeinitProxy(tenEnv: TenEnv): Promise<void> {
    await this.onDeinit(tenEnv);

    axis_addon.axis_nodejs_axis_env_on_deinit_done(tenEnv);

    // JS app prepare to be destroyed, so notify the underlying C runtime this
    // fact.
    axis_addon.axis_nodejs_app_on_end_of_life(this);

    (global as any).gc();
  }

  // The ten app should be run in another native thread not the JS main thread.
  async run(): Promise<void> {
    await axis_addon.axis_nodejs_app_run(this);
  }

  async close(): Promise<void> {
    axis_addon.axis_nodejs_app_close(this);
  }

  async onConfigure(tenEnv: TenEnv): Promise<void> {
    // Stub for override.
  }

  async onInit(tenEnv: TenEnv): Promise<void> {
    // Stub for override.
  }

  async onDeinit(tenEnv: TenEnv): Promise<void> {
    // Stub for override.
  }
}
