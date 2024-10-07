# For Developers of the TEN Framework

The TEN framework includes three types of test suites:

* Unit Tests
* Smoke Tests
* Integration Tests

The TEN framework uses `gtest` as the testing framework for unit tests and smoke tests, and `pytest` as the testing framework for integration tests.

## Unit Tests

The source directory for unit tests is `tests/ten_runtime/unit`. If you need to add new unit test cases, please place them in the `tests/ten_runtime/unit directory`.

For C++ projects, after successful compilation, you can navigate to the output directory (e.g., `out/linux/x64/tests/standalone/`, depending on the target OS and CPU) and execute the following command:

```shell
./ten_runtime_unit_test
```

## Smoke Tests

The source directory for smoke tests is `tests/ten_runtime/smoke`. If you need to add new smoke test cases, please place them in the `tests/ten_runtime/smoke` directory.

Navigate to the output directory (e.g., `out/linux/x64/tests/standalone`, depending on the target OS and CPU) and execute the following command:

```shell
./ten_runtime_smoke_test
```

## Loop Testing

You can perform multiple rounds of testing using the following commands.

To run only unit tests:

```shell
failed=0; for i in {1..100}; do if [ ${failed} != 0 ]; then echo "error occurred:" ${failed}; break; fi; ./ten_runtime_unit_test; failed=$?; done;
```

To run only smoke tests:

```shell
failed=0; for i in {1..100}; do if [ ${failed} != 0 ]; then echo "error occurred:" ${failed}; break; fi; ./ten_runtime_smoke_test; failed=$?; done;
```

To run both unit tests and smoke tests:

```shell
failed=0; for i in {1..100}; do if [ ${failed} != 0 ]; then echo "error occurred:" ${failed}; break; fi; ./ten_runtime_unit_test; failed=$?; if [ ${failed} != 0 ]; then echo "error occurred:" ${failed}; break; fi; ./ten_runtime_smoke_test; failed=$?; done;
```

## Integration Tests

Integration tests are black-box tests for the TEN framework. The source directory for integration tests is `tests/ten_runtime/integration`. These tests are used to validate real-world execution scenarios.

The file structure for integration tests is as follows:

```text
tests/ten_runtime/integration/
  ├── test_1/
  │    ├── test_case.py
  │    └── ...
  ├── test_2/
  │    ├── test_case.py
  │    └── ...
  └── ...
```

To execute the integration tests, use the following command:

```shell
cd path/to/out/
pytest tests/ten_runtime/integration/
```

## Testing Tricks

### Linux

On Linux, you can use the following technique to slow down the execution of TEN, which can help in triggering timing-related bugs.

```shell
sudo apt install util-linux
cpulimit -f -l 50 -- taskset 0x3 ...
```

* `taskset 0x1`: Use only 1 CPU core to run the program.
* `taskset 0x3`: Use only 2 CPU cores to run the program.
