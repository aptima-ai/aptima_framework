#!/bin/bash

SCRIPT_DIR=$(
  cd "$(dirname "$0")" || exit
  pwd
)

PROJECT_ROOT_DIR=$(
  cd "$SCRIPT_DIR/.." || exit
  pwd
)

# 檢查是否提供了參數
if [ -z "$1" ]; then
  echo "Error: Missing base directory argument."
  echo "Usage: $0 <base-dir>"
  exit 1
fi

BASE_DIR=$1

# Change to `frontend` folder and build the frontend.
echo "Building frontend..."
cd "$PROJECT_ROOT_DIR/frontend" || exit 1
npm install || exit 1
npm run build || exit 1

# Change back to the project root folder.
cd "$PROJECT_ROOT_DIR" || exit 1

# Build tman rust project.
echo "Building and running tman dev-server ..."
cargo run dev-server --base-dir="$BASE_DIR"
