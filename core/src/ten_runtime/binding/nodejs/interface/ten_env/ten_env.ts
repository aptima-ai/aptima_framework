//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import { AudioFrame } from "../msg/audio_frame";
import { VideoFrame } from "../msg/video_frame";
import { Cmd } from "../msg/cmd";
import { CmdResult } from "../msg/cmd_result";
import { Data } from "../msg/data";
import ten_addon from "../ten_addon";
import { LogLevel } from "./log_level";

export class TenEnv {
  async sendCmd(cmd: Cmd): Promise<[CmdResult | null, Error | null]> {
    return new Promise<[CmdResult | null, Error | null]>((resolve) => {
      ten_addon.ten_nodejs_ten_env_send_cmd(
        this,
        cmd,
        async (cmdResult: CmdResult, error: Error) => {
          resolve([cmdResult, error]);
        }
      );
    });
  }

  async sendData(data: Data): Promise<Error | null> {
    return new Promise<Error | null>((resolve) => {
      ten_addon.ten_nodejs_ten_env_send_data(
        this,
        data,
        async (error: Error) => {
          resolve(error);
        }
      );
    });
  }

  async sendVideoFrame(videoFrame: VideoFrame): Promise<Error | null> {
    return new Promise<Error | null>((resolve) => {
      ten_addon.ten_nodejs_ten_env_send_video_frame(
        this,
        videoFrame,
        async (error: Error) => {
          resolve(error);
        }
      );
    });
  }

  async sendAudioFrame(audioFrame: AudioFrame): Promise<Error | null> {
    return new Promise<Error | null>((resolve) => {
      ten_addon.ten_nodejs_ten_env_send_audio_frame(
        this,
        audioFrame,
        async (error: Error) => {
          resolve(error);
        }
      );
    });
  }

  async returnResult(
    cmdResult: CmdResult,
    targetCmd: Cmd
  ): Promise<Error | null> {
    return new Promise<Error | null>((resolve) => {
      ten_addon.ten_nodejs_ten_env_return_result(
        this,
        cmdResult,
        targetCmd,
        async (error: Error) => {
          resolve(error);
        }
      );
    });
  }

  async returnResultDirectly(cmdResult: CmdResult): Promise<Error | null> {
    return new Promise<Error | null>((resolve) => {
      ten_addon.ten_nodejs_ten_env_return_result_directly(
        this,
        cmdResult,
        async (error: Error) => {
          resolve(error);
        }
      );
    });
  }

  async isPropertyExist(path: string): Promise<boolean> {
    return new Promise<boolean>((resolve) => {
      ten_addon.ten_nodejs_ten_env_is_property_exist(
        this,
        path,
        async (result: boolean) => {
          resolve(result);
        }
      );
    });
  }

  async getPropertyToJson(path: string): Promise<string> {
    return new Promise<string>((resolve, reject) => {
      ten_addon.ten_nodejs_ten_env_get_property_to_json(
        this,
        path,
        async (result: string, error: Error) => {
          if (error) {
            reject(error);
          } else {
            resolve(result);
          }
        }
      );
    });
  }

  async setPropertyFromJson(path: string, jsonStr: string): Promise<void> {
    return new Promise<void>((resolve, reject) => {
      ten_addon.ten_nodejs_ten_env_set_property_from_json(
        this,
        path,
        jsonStr,
        async (error: Error) => {
          if (error) {
            reject(error);
          } else {
            resolve();
          }
        }
      );
    });
  }

  async getPropertyNumber(path: string): Promise<number> {
    return new Promise<number>((resolve, reject) => {
      ten_addon.ten_nodejs_ten_env_get_property_number(
        this,
        path,
        async (result: number, error: Error) => {
          if (error) {
            reject(error);
          } else {
            resolve(result);
          }
        }
      );
    });
  }

  async setPropertyNumber(path: string, value: number): Promise<void> {
    return new Promise<void>((resolve, reject) => {
      ten_addon.ten_nodejs_ten_env_set_property_number(
        this,
        path,
        value,
        async (error: Error) => {
          if (error) {
            reject(error);
          } else {
            resolve();
          }
        }
      );
    });
  }

  async getPropertyString(path: string): Promise<string> {
    return new Promise<string>((resolve, reject) => {
      ten_addon.ten_nodejs_ten_env_get_property_string(
        this,
        path,
        async (result: string, error: Error) => {
          if (error) {
            reject(error);
          } else {
            resolve(result);
          }
        }
      );
    });
  }

  async setPropertyString(path: string, value: string): Promise<void> {
    return new Promise<void>((resolve, reject) => {
      ten_addon.ten_nodejs_ten_env_set_property_string(
        this,
        path,
        value,
        async (error: Error) => {
          if (error) {
            reject(error);
          } else {
            resolve();
          }
        }
      );
    });
  }

  async initPropertyFromJson(jsonStr: string): Promise<void> {
    return new Promise<void>((resolve, reject) => {
      ten_addon.ten_nodejs_ten_env_init_property_from_json(
        this,
        jsonStr,
        async (error: Error) => {
          if (error) {
            reject(error);
          } else {
            resolve();
          }
        }
      );
    });
  }

  logVerbose(message: string): void {
    this.log_internal(LogLevel.VERBOSE, message);
  }

  logDebug(message: string): void {
    this.log_internal(LogLevel.DEBUG, message);
  }

  logInfo(message: string): void {
    this.log_internal(LogLevel.INFO, message);
  }

  logWarn(message: string): void {
    this.log_internal(LogLevel.WARN, message);
  }

  logError(message: string): void {
    this.log_internal(LogLevel.ERROR, message);
  }

  logFatal(message: string): void {
    this.log_internal(LogLevel.FATAL, message);
  }

  private log_internal(level: number, message: string): void {
    const _prepareStackTrace = Error.prepareStackTrace;
    Error.prepareStackTrace = (_, stack): NodeJS.CallSite[] => stack;
    const stack_ = new Error().stack as unknown as NodeJS.CallSite[];
    const stack = stack_.slice(1);
    Error.prepareStackTrace = _prepareStackTrace;

    const _callerFile = stack[1].getFileName();
    const _callerLine = stack[1].getLineNumber();
    const _callerFunction = stack[1].getFunctionName();

    const callerFile = _callerFile ? _callerFile : "unknown";
    const callerLine = _callerLine ? _callerLine : 0;
    const callerFunction = _callerFunction ? _callerFunction : "anonymous";

    ten_addon.ten_nodejs_ten_env_log_internal(
      this,
      level,
      callerFunction,
      callerFile,
      callerLine,
      message
    );
  }
}

ten_addon.ten_nodejs_ten_env_register_class(TenEnv);
