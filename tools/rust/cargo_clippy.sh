#!/bin/bash

cd core/src/ten_rust || exit 1
cargo clippy -- -D warnings

cd ../../..

cd core/src/ten_manager || exit 1
cargo clippy -- -D warnings
