#!/bin/bash

SCRIPT_DIR=$(
  cd "$(dirname "$0")" || exit
  pwd
)

PROJECT_ROOT_DIR=$(
  cd "$SCRIPT_DIR/.." || exit
  pwd
)

# Change to `frontend` folder and build the frontend.
echo "Building frontend..."
cd "$PROJECT_ROOT_DIR/frontend" || exit 1
npm install || exit 1
npm run build || exit 1

# Change back to the project root folder.
cd "$PROJECT_ROOT_DIR" || exit 1

# Build tman rust project.
echo "Building ..."
cargo build --release || exit 1
