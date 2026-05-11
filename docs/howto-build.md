# Build NVECode from source

NVECode is built the same way VSCodium is built: the upstream
[`microsoft/vscode`](https://github.com/microsoft/vscode) source tree is cloned at a
pinned commit, branded via `prepare_vscode.sh`, the NVECode patches in `patches/` are
applied, and then the standard `vscode` gulp / esbuild pipeline runs.

## Prerequisites

| Component | Linux / WSL              | macOS                    | Windows                  |
|-----------|--------------------------|--------------------------|--------------------------|
| Node.js   | 22.x (`.nvmrc`)          | 22.x                     | 22.x                     |
| npm       | bundled with node        | bundled                  | bundled                  |
| Python    | 3.10+                    | 3.10+ (`python3`)        | 3.10+                    |
| jq        | `apt install jq`         | `brew install jq`        | bundled with Git Bash    |
| Rust      | 1.95+ for the `vecode-tunnel` CLI | 1.95+           | 1.95+                    |
| C / C++   | gcc 10+ / clang          | Xcode CLT                | VS Build Tools 2022      |
| Inno Setup| —                        | —                        | 6.x for `vecode.iss`     |

The build also pulls Microsoft's open-source `vscode` repo into `./vscode/` and a bunch
of npm dependencies. Expect ~6 GB of working space.

## Linux / macOS

```bash
# clean build, stable channel, MS Marketplace
./dev/build.sh

# insider channel
./dev/build.sh -i

# offline mode (no remote gallery at all)
./dev/build.sh -m offline

# package installers as well as the binary
./dev/build.sh -p
```

Artifacts:

| OS     | Path                                            |
|--------|-------------------------------------------------|
| Linux  | `VSCode-linux-${ARCH}/`, plus `assets/*.deb`, `assets/*.rpm`, `assets/*.AppImage` |
| macOS  | `VSCode-darwin-${ARCH}/NVECode.app`             |

## Windows

Open Git Bash:

```bash
"C:\Program Files\Git\bin\bash.exe" ./dev/build.sh
```

This drops `VSCode-win32-${ARCH}/` containing `vecode.exe`. Then run Inno Setup:

```
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" build\win32\vecode.iss
```

This produces `vecode-${VERSION}-${ARCH}-setup.exe`.

## Environment variables

All branding lives in [`utils.sh`](../utils.sh). The build pipeline never hard-codes a
brand string — every patch goes through `apply_patch` which substitutes:

| Placeholder              | Default                                  |
|--------------------------|------------------------------------------|
| `!!APP_NAME!!`           | `NVECode`                                |
| `!!APP_NAME_LC!!`        | `nvecode`                                |
| `!!BINARY_NAME!!`        | `vecode`                                 |
| `!!ORG_NAME!!`           | `Neofilisoft`                            |
| `!!ASSETS_REPOSITORY!!`  | `Neofilisoft/nvecode`                    |
| `!!GH_REPO_PATH!!`       | `Neofilisoft/nvecode`                    |
| `!!PRODUCT_LONG_NAME!!`  | `Neofilisoft Visual Editor Code`         |

Override any of them as shell env vars to do a vendor-rebrand without editing source:

```bash
APP_NAME=AcmeCode BINARY_NAME=acode ORG_NAME=Acme ./dev/build.sh
```
