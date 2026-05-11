#!/usr/bin/env bash
# shellcheck disable=SC1091,2154

set -e

. ./utils.sh

if [[ "${VSCODE_QUALITY}" == "insider" ]]; then
  cp -rp src/insider/* vscode/
else
  cp -rp src/stable/* vscode/
fi

cp -f LICENSE vscode/LICENSE.txt

cd vscode || { echo "'vscode' dir not found"; exit 1; }

rm -rf extensions/copilot

{ set +x; } 2>/dev/null

# {{{ product.json
cp product.json{,.bak}

setpath() {
  local jsonTmp
  { set +x; } 2>/dev/null
  jsonTmp=$( jq --arg 'value' "${3}" "setpath(path(.${2}); \$value)" "${1}.json" )
  echo "${jsonTmp}" > "${1}.json"
  set -x
}

setpath_json() {
  local jsonTmp
  { set +x; } 2>/dev/null
  jsonTmp=$( jq --argjson 'value' "${3}" "setpath(path(.${2}); \$value)" "${1}.json" )
  echo "${jsonTmp}" > "${1}.json"
  set -x
}

# Marketplace selection.
#   - vscode-marketplace : Microsoft's Visual Studio Marketplace (same backend Windsurf uses).
#   - open-vsx          : Eclipse Open VSX (FOSS gallery).
#   - offline           : no remote gallery; only local .vsix installs.
# The runtime telemetry/offline toggle still lets the end user opt out.
MARKETPLACE_BACKEND="${MARKETPLACE_BACKEND:-vscode-marketplace}"

case "${MARKETPLACE_BACKEND}" in
  vscode-marketplace)
    setpath_json "product" "extensionsGallery" '{"serviceUrl": "https://marketplace.visualstudio.com/_apis/public/gallery", "cacheUrl": "https://vscode.blob.core.windows.net/gallery/index", "itemUrl": "https://marketplace.visualstudio.com/items", "publisherUrl": "https://marketplace.visualstudio.com/publishers", "resourceUrlTemplate": "https://{publisher}.gallery.vsassets.io/_apis/public/gallery/publisher/{publisher}/extension/{name}/{version}/assetbyname/{path}", "controlUrl": "https://main.vscode-cdn.net/extensions/marketplace.json", "nlsBaseUrl": "https://www.vscode-unpkg.net/_lp/", "extensionUrlTemplate": "https://www.vscode-unpkg.net/_gallery/{publisher}/{name}/latest"}'
    setpath_json "product" "linkProtectionTrustedDomains" '["https://marketplace.visualstudio.com", "https://open-vsx.org"]'
    ;;
  open-vsx)
    setpath_json "product" "extensionsGallery" '{"serviceUrl": "https://open-vsx.org/vscode/gallery", "itemUrl": "https://open-vsx.org/vscode/item", "latestUrlTemplate": "https://open-vsx.org/vscode/gallery/{publisher}/{name}/latest", "controlUrl": "https://raw.githubusercontent.com/EclipseFdn/publish-extensions/refs/heads/master/extension-control/extensions.json"}'
    setpath_json "product" "linkProtectionTrustedDomains" '["https://open-vsx.org"]'
    ;;
  offline|none)
    setpath_json "product" "extensionsGallery" '{}'
    setpath_json "product" "linkProtectionTrustedDomains" '[]'
    ;;
  *)
    echo "Unknown MARKETPLACE_BACKEND='${MARKETPLACE_BACKEND}', defaulting to open-vsx"
    setpath_json "product" "extensionsGallery" '{"serviceUrl": "https://open-vsx.org/vscode/gallery", "itemUrl": "https://open-vsx.org/vscode/item", "latestUrlTemplate": "https://open-vsx.org/vscode/gallery/{publisher}/{name}/latest"}'
    setpath_json "product" "linkProtectionTrustedDomains" '["https://open-vsx.org"]'
    ;;
esac

setpath "product" "checksumFailMoreInfoUrl" "https://github.com/${GH_REPO_PATH}/blob/main/docs/troubleshooting.md"
setpath "product" "documentationUrl" "https://github.com/${GH_REPO_PATH}#readme"
setpath "product" "introductoryVideosUrl" "https://github.com/${GH_REPO_PATH}#readme"
setpath "product" "keyboardShortcutsUrlLinux" "https://github.com/${GH_REPO_PATH}/blob/main/docs/keyboard-shortcuts-linux.md"
setpath "product" "keyboardShortcutsUrlMac" "https://github.com/${GH_REPO_PATH}/blob/main/docs/keyboard-shortcuts-macos.md"
setpath "product" "keyboardShortcutsUrlWin" "https://github.com/${GH_REPO_PATH}/blob/main/docs/keyboard-shortcuts-windows.md"
setpath "product" "licenseUrl" "https://github.com/${GH_REPO_PATH}/blob/main/LICENSE"
setpath "product" "releaseNotesUrl" "https://github.com/${GH_REPO_PATH}/releases"
setpath "product" "reportIssueUrl" "https://github.com/${GH_REPO_PATH}/issues/new"
setpath "product" "requestFeatureUrl" "https://github.com/${GH_REPO_PATH}/issues/new"
setpath "product" "tipsAndTricksUrl" "https://github.com/${GH_REPO_PATH}#readme"
setpath "product" "twitterUrl" ""

if [[ "${DISABLE_UPDATE}" != "yes" ]]; then
  setpath "product" "updateUrl" "https://raw.githubusercontent.com/${ORG_NAME}/versions/refs/heads/main"

  if [[ "${VSCODE_QUALITY}" == "insider" ]]; then
    setpath "product" "downloadUrl" "https://github.com/${ORG_NAME}/nvecode-insiders/releases"
  else
    setpath "product" "downloadUrl" "https://github.com/${GH_REPO_PATH}/releases"
  fi
fi

if [[ "${VSCODE_QUALITY}" == "insider" ]]; then
  setpath "product" "nameShort" "${APP_NAME} - Insiders"
  setpath "product" "nameLong" "${PRODUCT_LONG_NAME} - Insiders"
  setpath "product" "applicationName" "${BINARY_NAME}-insiders"
  setpath "product" "dataFolderName" ".${APP_NAME_LC}-insiders"
  setpath "product" "linuxIconName" "${APP_NAME_LC}-insiders"
  setpath "product" "quality" "insider"
  setpath "product" "urlProtocol" "${APP_NAME_LC}-insiders"
  setpath "product" "serverApplicationName" "${BINARY_NAME}-server-insiders"
  setpath "product" "serverDataFolderName" ".${APP_NAME_LC}-server-insiders"
  setpath "product" "darwinBundleIdentifier" "com.${ORG_NAME,,}.${APP_NAME}Insiders"
  setpath "product" "win32AppUserModelId" "${ORG_NAME}.${APP_NAME}Insiders"
  setpath "product" "win32DirName" "${APP_NAME} Insiders"
  setpath "product" "win32MutexName" "${APP_NAME_LC}insiders"
  setpath "product" "win32NameVersion" "${APP_NAME} Insiders"
  setpath "product" "win32RegValueName" "${APP_NAME}Insiders"
  setpath "product" "win32ShellNameShort" "${APP_NAME} Insiders"
  setpath "product" "win32AppId" "{{8B7215EF-2C2F-4F66-863F-E32ECF9D06B5}"
  setpath "product" "win32x64AppId" "{{BA4B0925-893A-42AE-A01A-457B8EAED5D2}"
  setpath "product" "win32arm64AppId" "{{2696D89D-AC7B-48CF-B8FD-EBA2E1AB81DC}"
  setpath "product" "win32UserAppId" "{{76C54293-2593-49B9-BDD2-D1975A52A9E8}"
  setpath "product" "win32x64UserAppId" "{{36CC2929-8688-4A05-BC38-3780BEE3302E}"
  setpath "product" "win32arm64UserAppId" "{{5208E34A-FEEF-49F5-9B1F-8194B46271AF}"
  setpath "product" "tunnelApplicationName" "${BINARY_NAME}-insiders-tunnel"
  setpath "product" "win32TunnelServiceMutex" "${APP_NAME_LC}insiders-tunnelservice"
  setpath "product" "win32TunnelMutex" "${APP_NAME_LC}insiders-tunnel"
  setpath "product" "win32ContextMenu.x64.clsid" "1B1B0B50-CA7C-44A4-B782-84A6F51A8C11"
  setpath "product" "win32ContextMenu.arm64.clsid" "AE364F5A-1DFB-41CF-A38A-6128E8E18D86"
else
  setpath "product" "nameShort" "${APP_NAME}"
  setpath "product" "nameLong" "${PRODUCT_LONG_NAME}"
  setpath "product" "applicationName" "${BINARY_NAME}"
  setpath "product" "dataFolderName" ".${APP_NAME_LC}"
  setpath "product" "linuxIconName" "${APP_NAME_LC}"
  setpath "product" "quality" "stable"
  setpath "product" "urlProtocol" "${APP_NAME_LC}"
  setpath "product" "serverApplicationName" "${BINARY_NAME}-server"
  setpath "product" "serverDataFolderName" ".${APP_NAME_LC}-server"
  setpath "product" "darwinBundleIdentifier" "com.${ORG_NAME,,}.${APP_NAME}"
  setpath "product" "win32AppUserModelId" "${ORG_NAME}.${APP_NAME}"
  setpath "product" "win32DirName" "${APP_NAME}"
  setpath "product" "win32MutexName" "${APP_NAME_LC}"
  setpath "product" "win32NameVersion" "${APP_NAME}"
  setpath "product" "win32RegValueName" "${APP_NAME}"
  setpath "product" "win32ShellNameShort" "${APP_NAME}"
  setpath "product" "win32AppId" "{{79269904-5A90-4033-AAB1-D1D04EC743CA}"
  setpath "product" "win32x64AppId" "{{03BD9AA9-B5D7-470D-95C0-8B12F611D884}"
  setpath "product" "win32arm64AppId" "{{A6C7C2DC-9E09-48E2-9625-5C042A998375}"
  setpath "product" "win32UserAppId" "{{4C628DBB-2CC0-4372-AB4B-BCDAF8198723}"
  setpath "product" "win32x64UserAppId" "{{8B7215EF-2C2F-4F66-863F-E32ECF9D06B5}"
  setpath "product" "win32arm64UserAppId" "{{BA4B0925-893A-42AE-A01A-457B8EAED5D2}"
  setpath "product" "tunnelApplicationName" "${BINARY_NAME}-tunnel"
  setpath "product" "win32TunnelServiceMutex" "${APP_NAME_LC}-tunnelservice"
  setpath "product" "win32TunnelMutex" "${APP_NAME_LC}-tunnel"
  setpath "product" "win32ContextMenu.x64.clsid" "76C54293-2593-49B9-BDD2-D1975A52A9E8"
  setpath "product" "win32ContextMenu.arm64.clsid" "36CC2929-8688-4A05-BC38-3780BEE3302E"
fi

setpath_json "product" "tunnelApplicationConfig" '{}'

jsonTmp=$( jq -s '.[0] * .[1]' product.json ../product.json )
echo "${jsonTmp}" > product.json && unset jsonTmp

cat product.json
# }}}

# include common functions
. ../utils.sh

# {{{ apply patches

echo "APP_NAME=\"${APP_NAME}\""
echo "APP_NAME_LC=\"${APP_NAME_LC}\""
echo "ASSETS_REPOSITORY=\"${ASSETS_REPOSITORY}\""
echo "BINARY_NAME=\"${BINARY_NAME}\""
echo "GH_REPO_PATH=\"${GH_REPO_PATH}\""
echo "GLOBAL_DIRNAME=\"${GLOBAL_DIRNAME}\""
echo "ORG_NAME=\"${ORG_NAME}\""
echo "PRODUCT_LONG_NAME=\"${PRODUCT_LONG_NAME}\""
echo "PUBLISHER=\"${PUBLISHER}\""
echo "TUNNEL_APP_NAME=\"${TUNNEL_APP_NAME}\""

if [[ "${DISABLE_UPDATE}" == "yes" ]]; then
  if [[ -f ../patches/00-update-disable.patch.yet ]]; then
    mv ../patches/00-update-disable.patch.yet ../patches/00-update-disable.patch
  fi
fi

for file in ../patches/*.patch; do
  if [[ -f "${file}" ]]; then
    apply_patch "${file}"
  fi
done

if [[ "${VSCODE_QUALITY}" == "insider" ]]; then
  for file in ../patches/insider/*.patch; do
    if [[ -f "${file}" ]]; then
      apply_patch "${file}"
    fi
  done
fi

if [[ -d "../patches/${OS_NAME}/" ]]; then
  for file in "../patches/${OS_NAME}/"*.patch; do
    if [[ -f "${file}" ]]; then
      apply_patch "${file}"
    fi
  done
fi

for file in ../patches/user/*.patch; do
  if [[ -f "${file}" ]]; then
    apply_patch "${file}"
  fi
done
# }}}

# {{{ inject built-in extensions
# Built-in NVECode extensions live under ../src/${stable|insider}/builtin-extensions/
# Each one is copied into vscode/extensions/ before the main build picks them
# up and minifies them into the final app bundle. The same extensions are kept
# in both channels so we pick the right source tree based on VSCODE_QUALITY.
if [[ "${VSCODE_QUALITY}" == "insider" ]]; then
  BUILTIN_EXT_SRC="../src/insider/builtin-extensions"
else
  BUILTIN_EXT_SRC="../src/stable/builtin-extensions"
fi
if [[ -d "${BUILTIN_EXT_SRC}" ]]; then
  for ext_dir in "${BUILTIN_EXT_SRC}"/*/; do
    if [[ -d "${ext_dir}" ]]; then
      ext_name=$( basename "${ext_dir}" )
      echo "Injecting built-in extension: ${ext_name}"
      rm -rf "extensions/${ext_name}"
      cp -rp "${ext_dir}" "extensions/${ext_name}"
    fi
  done
fi
# }}}

set -x

# {{{ install dependencies
export ELECTRON_SKIP_BINARY_DOWNLOAD=1
export PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD=1

if [[ "${OS_NAME}" == "linux" ]]; then
  export VSCODE_SKIP_NODE_VERSION_CHECK=1

   if [[ "${npm_config_arch}" == "arm" ]]; then
    export npm_config_arm_version=7
  fi
elif [[ "${OS_NAME}" == "windows" ]]; then
  if [[ "${npm_config_arch}" == "arm" ]]; then
    export npm_config_arm_version=7
  fi
else
  if [[ "${CI_BUILD}" != "no" ]]; then
    clang++ --version
  fi
fi

node build/npm/preinstall.ts

mv .npmrc .npmrc.bak
cp ../npmrc .npmrc

for i in {1..5}; do # try 5 times
  if [[ "${CI_BUILD}" != "no" && "${OS_NAME}" == "osx" ]]; then
    CXX=clang++ npm ci && break
  else
    npm ci && break
  fi

  if [[ $i == 5 ]]; then
    echo "Npm install failed too many times" >&2
    exit 1
  fi
  echo "Npm install failed $i, trying again..."

  sleep $(( 15 * (i + 1)))
done

mv .npmrc.bak .npmrc
# }}}

# package.json
cp package.json{,.bak}

setpath "package" "version" "${RELEASE_VERSION%-insider}"

replace "s|Microsoft Corporation|${PUBLISHER}|" package.json

cp resources/server/manifest.json{,.bak}

if [[ "${VSCODE_QUALITY}" == "insider" ]]; then
  setpath "resources/server/manifest" "name" "${APP_NAME} - Insiders"
  setpath "resources/server/manifest" "short_name" "${APP_NAME} - Insiders"
else
  setpath "resources/server/manifest" "name" "${APP_NAME}"
  setpath "resources/server/manifest" "short_name" "${APP_NAME}"
fi

# announcements: replace the upstream BUILTIN_ANNOUNCEMENTS sentinel with ours
if [[ -f ../announcements-builtin.json ]]; then
  replace "s|\\[\\/\\* BUILTIN_ANNOUNCEMENTS \\*\\/\\]|$( tr -d '\n' < ../announcements-builtin.json )|" src/vs/workbench/contrib/welcomeGettingStarted/browser/gettingStarted.ts
fi

../undo_telemetry.sh

replace "s|Microsoft Corporation|${PUBLISHER}|" build/lib/electron.ts
replace "s|([0-9]) Microsoft|\\1 ${PUBLISHER}|" build/lib/electron.ts

if [[ "${OS_NAME}" == "linux" ]]; then
  # Replace the "code-oss" marker so the postinst script doesn't add the
  # upstream apt repo to the user's sources.
  if [[ "${VSCODE_QUALITY}" == "insider" ]]; then
    sed -i "s/code-oss/${BINARY_NAME}-insiders/" resources/linux/debian/postinst.template
  else
    sed -i "s/code-oss/${BINARY_NAME}/" resources/linux/debian/postinst.template
  fi

  # code.appdata.xml
  sed -i "s|Visual Studio Code|${APP_NAME}|g" resources/linux/code.appdata.xml
  sed -i "s|https://code.visualstudio.com/docs/setup/linux|https://github.com/${GH_REPO_PATH}#readme|" resources/linux/code.appdata.xml
  sed -i "s|https://code.visualstudio.com/home/home-screenshot-linux-lg.png|https://raw.githubusercontent.com/${GH_REPO_PATH}/main/icons/stable/${APP_NAME_LC}_cnl.svg|" resources/linux/code.appdata.xml
  sed -i "s|https://code.visualstudio.com|https://github.com/${GH_REPO_PATH}|" resources/linux/code.appdata.xml

  # debian/control.template
  sed -i "s|Microsoft Corporation <vscode-linux@microsoft.com>|${PUBLISHER} https://github.com/${GH_REPO_PATH}/graphs/contributors|"  resources/linux/debian/control.template
  sed -i "s|Visual Studio Code|${APP_NAME}|g" resources/linux/debian/control.template
  sed -i "s|https://code.visualstudio.com/docs/setup/linux|https://github.com/${GH_REPO_PATH}#readme|" resources/linux/debian/control.template
  sed -i "s|https://code.visualstudio.com|https://github.com/${GH_REPO_PATH}|" resources/linux/debian/control.template

  # rpm/code.spec.template
  sed -i "s|Microsoft Corporation|${PUBLISHER}|" resources/linux/rpm/code.spec.template
  sed -i "s|Visual Studio Code Team <vscode-linux@microsoft.com>|${PUBLISHER} https://github.com/${GH_REPO_PATH}/graphs/contributors|" resources/linux/rpm/code.spec.template
  sed -i "s|Visual Studio Code|${APP_NAME}|" resources/linux/rpm/code.spec.template
  sed -i "s|https://code.visualstudio.com/docs/setup/linux|https://github.com/${GH_REPO_PATH}#readme|" resources/linux/rpm/code.spec.template
  sed -i "s|https://code.visualstudio.com|https://github.com/${GH_REPO_PATH}|" resources/linux/rpm/code.spec.template
elif [[ "${OS_NAME}" == "windows" ]]; then
  # code.iss (upstream Inno Setup template). vecode.iss lives at build/win32/vecode.iss
  # and replaces this one during packaging.
  sed -i "s|https://code.visualstudio.com|https://github.com/${GH_REPO_PATH}|" build/win32/code.iss
  sed -i "s|Microsoft Corporation|${PUBLISHER}|" build/win32/code.iss
fi

cd ..
