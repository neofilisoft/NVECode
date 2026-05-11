#!/usr/bin/env bash
# shellcheck disable=SC1091
#
# Local helper to build the CLI/tunnel binary and launch a dev `serve-web`
# session against a locally built NVECode Insiders.app on macOS arm64.
# Adjust the paths if you build for a different platform / quality.

. ./utils.sh

export CARGO_NET_GIT_FETCH_WITH_CLI="true"
export VSCODE_CLI_APP_NAME="${APP_NAME_LC}"
export VSCODE_CLI_BINARY_NAME="${BINARY_NAME}-server-insiders"
export VSCODE_CLI_DOWNLOAD_URL="https://github.com/${ORG_NAME}/nvecode-insiders/releases"
export VSCODE_CLI_QUALITY="insider"
export VSCODE_CLI_UPDATE_URL="https://raw.githubusercontent.com/${ORG_NAME}/versions/refs/heads/main"

cargo build --release --target aarch64-apple-darwin --bin=code

cp target/aarch64-apple-darwin/release/code "../../VSCode-darwin-arm64/${APP_NAME} - Insiders.app/Contents/Resources/app/bin/${BINARY_NAME}-tunnel-insiders"

"../../VSCode-darwin-arm64/${APP_NAME} - Insiders.app/Contents/Resources/app/bin/${BINARY_NAME}-insiders" serve-web
