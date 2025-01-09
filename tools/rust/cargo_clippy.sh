#!/bin/bash

cd core/src/aptima_rust || exit 1
cargo clippy --all-features --tests -- -D warnings -W clippy::all
cargo clippy --release --all-features --tests -- -D warnings -W clippy::all

cd ../../..

cd core/src/aptima_manager || exit 1
cargo clippy --all-features --tests -- -D warnings -W clippy::all
cargo clippy --release --all-features --tests -- -D warnings -W clippy::all
