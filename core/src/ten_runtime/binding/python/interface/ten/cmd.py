#
# This file is part of the TEN Framework project.
# See https://github.com/TEN-framework/ten_framework/LICENSE for license
# information.
#
import json
import warnings
from libten_runtime_python import _Cmd


class Cmd(_Cmd):
    def __new__(cls, name: str):
        return super().__new__(cls, name)

    def __init__(self):
        raise NotImplementedError("Use Cmd.create_from_json instead.")

    @classmethod
    def create(cls, name: str):
        return cls.__new__(cls, name)

    @classmethod
    def create_from_json(cls, json_str: str):
        warnings.warn(
            (
                "This method may access the '_ten' field. "
                "Use caution if '_ten' is provided."
            ),
            UserWarning,
        )

        try:
            data = json.loads(json_str)
        except json.JSONDecodeError:
            # The JSON is invalid.
            return None

        ten_data = data.get("_ten")
        if not isinstance(ten_data, dict):
            # '_ten' is missing or not an object.
            return None

        if not isinstance(ten_data.get("name"), str):
            # 'name' is missing or not a string.
            return None

        # Extract the 'name' field from the '_ten' object
        name = ten_data["name"]

        instance = cls.__new__(cls, name)
        instance.from_json(json_str)
        return instance
