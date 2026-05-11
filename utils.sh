#!/usr/bin/env bash

# Default branding values for NVECode (Neofilisoft Visual Editor Code).
# Anything that names the product, the binary, or the publisher should be
# threaded through these variables so we never hardcode a brand string
# inside individual scripts or patches.

APP_NAME="${APP_NAME:-NVECode}"
APP_NAME_LC="$( echo "${APP_NAME}" | awk '{print tolower($0)}' )"
ASSETS_REPOSITORY="${ASSETS_REPOSITORY:-Neofilisoft/nvecode}"
BINARY_NAME="${BINARY_NAME:-vecode}"
GH_REPO_PATH="${GH_REPO_PATH:-Neofilisoft/nvecode}"
ORG_NAME="${ORG_NAME:-Neofilisoft}"
PUBLISHER="${PUBLISHER:-Neofilisoft}"
PRODUCT_LONG_NAME="${PRODUCT_LONG_NAME:-Neofilisoft Visual Editor Code}"
TUNNEL_APP_NAME="${TUNNEL_APP_NAME:-"${BINARY_NAME}-tunnel"}"

if [[ "${VSCODE_QUALITY}" == "insider" ]]; then
  GLOBAL_DIRNAME="${GLOBAL_DIRNAME:-"${APP_NAME_LC}"}-insiders"
else
  GLOBAL_DIRNAME="${GLOBAL_DIRNAME:-"${APP_NAME_LC}"}"
fi

apply_patch() {
  if [[ -z "$2" ]]; then
    echo applying patch: "$1";
  fi

  cp $1{,.bak}

  replace "s|!!APP_NAME!!|${APP_NAME}|g" "$1"
  replace "s|!!APP_NAME_LC!!|${APP_NAME_LC}|g" "$1"
  replace "s|!!ASSETS_REPOSITORY!!|${ASSETS_REPOSITORY}|g" "$1"
  replace "s|!!BINARY_NAME!!|${BINARY_NAME}|g" "$1"
  replace "s|!!GH_REPO_PATH!!|${GH_REPO_PATH}|g" "$1"
  replace "s|!!GLOBAL_DIRNAME!!|${GLOBAL_DIRNAME}|g" "$1"
  replace "s|!!ORG_NAME!!|${ORG_NAME}|g" "$1"
  replace "s|!!PUBLISHER!!|${PUBLISHER}|g" "$1"
  replace "s|!!PRODUCT_LONG_NAME!!|${PRODUCT_LONG_NAME}|g" "$1"
  replace "s|!!RELEASE_VERSION!!|${RELEASE_VERSION}|g" "$1"
  replace "s|!!TUNNEL_APP_NAME!!|${TUNNEL_APP_NAME}|g" "$1"

  if ! git apply --ignore-whitespace "$1"; then
    echo failed to apply patch "$1" >&2
    exit 1
  fi

  mv -f $1{.bak,}
}

exists() { type -t "$1" &> /dev/null; }

is_gnu_sed() {
  sed --version &> /dev/null
}

replace() {
  if is_gnu_sed; then
    sed -i -E "${1}" "${2}"
  else
    sed -i '' -E "${1}" "${2}"
  fi
}

if ! exists gsed; then
  if is_gnu_sed; then
    function gsed() {
      sed -i -E "$@"
    }
  else
    function gsed() {
      sed -i '' -E "$@"
    }
  fi
fi
