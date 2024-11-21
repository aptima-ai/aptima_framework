#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from libten_runtime_python import _Msg


class Msg(_Msg):
    def to_json(self) -> str:
        return _Msg.to_json(self)

    def from_json(self, json_str: str):
        return _Msg.from_json(self, json_str)

    def get_name(self) -> str:
        return _Msg.get_name(self)

    def set_dest(
        self,
        app_uri: str | None,
        graph_id: str | None,
        extension_group: str | None,
        extension: str | None,
    ) -> None:
        return _Msg.set_dest(
            self, app_uri, graph_id, extension_group, extension
        )

    def set_property_from_json(self, key: str, json: str) -> None:
        return _Msg.set_property_from_json(self, key, json)

    def get_property_to_json(self, key: str) -> str:
        return _Msg.get_property_to_json(self, key)

    def set_property_string(self, key: str, value: str) -> None:
        return _Msg.set_property_string(self, key, value)

    def get_property_string(self, key: str) -> str:
        return _Msg.get_property_string(self, key)

    def get_property_int(self, key: str) -> int:
        return _Msg.get_property_int(self, key)

    def set_property_int(self, key: str, value: int) -> None:
        return _Msg.set_property_int(self, key, value)

    def get_property_bool(self, key: str) -> bool:
        return _Msg.get_property_bool(self, key)

    def set_property_bool(self, key: str, value: bool) -> None:
        return _Msg.set_property_bool(self, key, value)

    def get_property_float(self, path: str) -> float:
        return _Msg.get_property_float(self, path)

    def set_property_float(self, path: str, value: float) -> None:
        return _Msg.set_property_float(self, path, value)

    def get_property_buf(self, path: str) -> bytearray:
        return _Msg.get_property_buf(self, path)

    def set_property_buf(self, path: str, value: bytes) -> None:
        return _Msg.set_property_buf(self, path, value)
