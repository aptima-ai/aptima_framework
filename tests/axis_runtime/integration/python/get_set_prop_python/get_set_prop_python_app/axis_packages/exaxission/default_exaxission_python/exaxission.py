#
# Copyright © 2025 Agora
# This file is part of APTIMA Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import threading
from typing import Optional

from aptima import Cmd, CmdResult, Extension, StatusCode, TenEnv, TenError


class DefaultExtension(Extension):
    def on_configure(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_init")

        aptima_env.init_property_from_json('{"testKey": "testValue"}')
        aptima_env.on_configure_done()

    def __test_thread_routine(self, aptima_env: TenEnv):
        i = 0
        for _ in range(0, 10000):
            try:
                throw_exception = False
                _ = aptima_env.get_property_string("undefinedKey")
            except Exception:
                i += 1
                throw_exception = True

            assert throw_exception is True

        assert i == 10000
        aptima_env.log_info("__test_thread_routine done")

    def on_start(self, aptima_env: TenEnv) -> None:
        aptima_env.log_debug("on_start")

        assert aptima_env.is_property_exist("env_not_set_has_default") is True

        try:
            env_value = aptima_env.get_property_string(
                "env_not_set_has_default"
            )
            assert env_value == ""
        except Exception as e:
            aptima_env.log_info(str(e))
            assert False

        assert aptima_env.is_property_exist("undefined_key") is False

        aptima_env.set_property_from_json("testKey2", '"testValue2"')

        assert aptima_env.is_property_exist("testKey") is True
        assert aptima_env.is_property_exist("testKey2") is True
        testValue = aptima_env.get_property_to_json("testKey")
        testValue2 = aptima_env.get_property_to_json("testKey2")
        assert testValue == '"testValue"'
        assert testValue2 == '"testValue2"'

        aptima_env.set_property_bool("testBoolTrue", True)
        aptima_env.set_property_bool("testBoolFalse", False)
        assert aptima_env.get_property_bool("testBoolTrue") is True
        assert aptima_env.get_property_bool("testBoolFalse") is False

        aptima_env.set_property_int("testInt", 123)
        assert aptima_env.get_property_int("testInt") == 123

        aptima_env.set_property_float("testFloat", 123.456)
        assert aptima_env.get_property_float("testFloat") == 123.456

        aptima_env.set_property_string("testString", "testString")
        assert aptima_env.get_property_string("testString") == "testString"

        self.thread_test = threading.Thread(
            target=self.__test_thread_routine, args=(aptima_env,)
        )

        self.thread_test.start()

        for _ in range(0, 10000):
            try:
                throw_exception = False
                _ = aptima_env.get_property_string("undefinedKey")
            except Exception:
                throw_exception = True

            assert throw_exception is True

        aptima_env.on_start_done()

    def __join_thread(self, aptima_env: TenEnv):
        if self.thread_test.is_alive():
            self.thread_test.join()

        aptima_env.on_stop_done()

    def on_stop(self, aptima_env: TenEnv) -> None:
        aptima_env.log_info("on_stop")

        # Start a new thread to join the previous thread to avoid blocking the
        # APTIMA extension thread.
        threading.Thread(target=self.__join_thread, args=(aptima_env,)).start()

    def on_deinit(self, aptima_env: TenEnv) -> None:
        aptima_env.log_info("on_deinit")
        aptima_env.on_deinit_done()

    def check_hello(
        self,
        aptima_env: TenEnv,
        result: Optional[CmdResult],
        error: Optional[TenError],
        receivedCmd: Cmd,
    ):
        if error is not None:
            assert False, error

        assert result is not None

        statusCode = result.get_status_code()
        detail = result.get_property_string("detail")
        aptima_env.log_info(
            "check_hello: status:" + str(statusCode) + " detail:" + detail
        )

        for i in range(0, 10000):
            try:
                throw_exception = False
                _ = result.get_property_string("undefinedKey")
            except Exception:
                throw_exception = True

            assert throw_exception is True

        respCmd = CmdResult.create(StatusCode.OK)
        respCmd.set_property_string("detail", detail + " nbnb")
        aptima_env.log_info("create respCmd")

        aptima_env.return_result(respCmd, receivedCmd)

    def on_cmd(self, aptima_env: TenEnv, cmd: Cmd) -> None:
        aptima_env.log_info("on_cmd")
        cmd_json = cmd.get_property_to_json()
        aptima_env.log_info("on_cmd json: " + cmd_json)

        new_cmd = Cmd.create("hello")
        new_cmd.set_property_from_json("test", '"testValue2"')
        test_value = new_cmd.get_property_to_json("test")
        assert test_value == '"testValue2"'

        new_cmd.set_property_bool("testBoolTrue", True)
        new_cmd.set_property_bool("testBoolFalse", False)
        assert new_cmd.get_property_bool("testBoolTrue") is True
        assert new_cmd.get_property_bool("testBoolFalse") is False

        new_cmd.set_property_int("testInt", 123)
        assert new_cmd.get_property_int("testInt") == 123

        new_cmd.set_property_float("testFloat", 123.456)
        assert new_cmd.get_property_float("testFloat") == 123.456

        new_cmd.set_property_string("testString", "testString")
        assert new_cmd.get_property_string("testString") == "testString"

        new_cmd.set_property_buf("testBuf", b"testBuf")
        assert new_cmd.get_property_buf("testBuf") == b"testBuf"

        try:
            _ = new_cmd.get_property_string("undefinedKey")
        except Exception as e:
            aptima_env.log_info(
                "DefaultExtension on_cmd get_property_string exception: "
                + str(e)
            )

        aptima_env.send_cmd(
            new_cmd,
            lambda aptima_env, result, error: self.check_hello(
                aptima_env, result, error, cmd
            ),
        )
