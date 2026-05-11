# NVECode - Neofilisoft Visual Editor Code

> Telemetry-free, AI-augmented distribution of Visual Studio Code.

NVECode is a [Neofilisoft](https://github.com/neofilisoft/NVECode) rebuild of the open-source
[`microsoft/vscode`](https://github.com/microsoft/vscode) source tree. It is heavily
inspired by — and structurally derived from — [VSCodium](https://github.com/VSCodium/vscodium).
NVECode keeps the VSCodium philosophy of "FOSS binaries with telemetry disabled" and
adds first-class support for:

- A pluggable **AI Agent** sidebar (BYO OpenAI / Anthropic / Ollama / OpenAI-compatible endpoint).
- A multi-provider **sign-in flow** (Google, GitHub, GitLab, or stay anonymous as Guest).
- A **marketplace mode switch** at the bottom of the IDE (online / offline) plus a one-click telemetry kill.
- Automatic **toolchain detection** for Rust 1.95+ / Cargo, Lua 5.5, C++17 / C++20 (clang/gcc/cl.exe) and Python.
- A Windows **Inno Setup** installer (`vecode.iss`) producing `vecode.exe`.

The binary is named `vecode` on all platforms (`vecode.exe` on Windows).

## Source layout

```
nvecode/
├── utils.sh                    # central branding variables (APP_NAME, BINARY_NAME, ORG_NAME, …)
├── prepare_vscode.sh           # rewrites product.json + applies all patches
├── get_repo.sh / version.sh    # pulls upstream microsoft/vscode at a pinned commit
├── build.sh / build_cli.sh     # platform-aware build entry points
├── prepare_assets.sh           # packages installers per OS
├── patches/                    # custom patches applied on top of vscode
│   ├── 00-nve-ai-agent.patch          # pluggable AI Agent
│   ├── 00-nve-auth.patch              # Google / GitHub / GitLab / Guest auth
│   ├── 00-nve-marketplace-offline.patch
│   ├── 00-nve-toolchain-detect.patch  # Rust / Lua / C++ / Python detection
│   └── …
├── src/
│   ├── stable/                 # stable-channel overlay (icons, plist, desktop files, builtin ext.)
│   │   └── builtin-extensions/ # NVE extensions baked into the build
│   └── insider/                # insider-channel overlay
├── build/win32/vecode.iss      # Inno Setup installer
├── build/linux/*               # AppImage / deb / rpm scripts
├── build/osx/*                 # macOS bundle scripts
├── icons/                      # NVECode (re-used VSCodium) icons
└── docs/                       # how-to-build, telemetry, toolchains, …
```

## Quick start

### Linux / macOS

```bash
./dev/build.sh                  # stable build
./dev/build.sh -i               # insider build
./dev/build.sh -m offline       # build with the extension gallery completely disabled
```

Build outputs land in `VSCode-${OS}-${ARCH}/`.

### Windows

Run inside Git Bash:

```bash
"C:\Program Files\Git\bin\bash.exe" ./dev/build.sh
```

Then compile the installer with Inno Setup:

```
ISCC.exe build\win32\vecode.iss
```

This produces `vecode-${VERSION}-${ARCH}.exe`.

### Pick a marketplace backend at build time

| `MARKETPLACE_BACKEND` | Gallery used                                  | Notes                                       |
|-----------------------|-----------------------------------------------|---------------------------------------------|
| `vscode-marketplace`  | Microsoft Visual Studio Marketplace (default) | Same backend Cursor / Windsurf use.         |
| `open-vsx`            | Eclipse Open VSX                              | The default VSCodium gallery.               |
| `offline` / `none`    | none                                          | Local `.vsix` install only.                 |

At runtime, the user can still flip **Settings → NVECode: Offline Mode** to disable all
remote gallery / telemetry traffic regardless of which gallery was baked in.

## AI Agent — pluggable provider

Open **View → AI Agent** (or `Ctrl+Alt+A` / `Cmd+Alt+A`). On first run NVECode asks for:

1. Provider — one of `openai-compatible`, `anthropic`, `ollama`, `openai`, `azure-openai`.
2. Base URL (only for `openai-compatible` / `ollama` / `azure-openai`).
3. API key (stored in the OS secret store; **never written to product.json**).
4. Model id.

No provider, key or endpoint is hard-coded anywhere in the source. See `docs/ai-agent.md`.

## Authentication

`Sign in` from the account picker supports:

- **Google** (OAuth 2.0 / device code) — set `NVE_GOOGLE_OAUTH_CLIENT_ID` at runtime.
- **GitHub** (OAuth) — set `NVE_GITHUB_OAUTH_CLIENT_ID`.
- **GitLab** (OAuth, hosted or self-managed) — set `NVE_GITLAB_OAUTH_CLIENT_ID` and `NVE_GITLAB_BASE_URL`.
- **Guest** — no network round-trip, profile data stays local.

OAuth client IDs are read from environment / user settings at runtime; nothing is baked
into the binary.

## Toolchain detection

On startup NVECode probes `PATH` for:

| Tool      | Probe command         | Recommended version |
|-----------|----------------------|---------------------|
| Rust      | `rustc --version`    | 1.95+               |
| Cargo     | `cargo --version`    | bundled with rustc  |
| Lua       | `lua -v`             | 5.5                 |
| Python    | `python --version` / `python3 --version` | 3.10+ |
| C++       | `clang++ -v` / `g++ -v` / `cl.exe`        | C++17 / C++20 |

Missing toolchains are reported via a non-modal notification with platform-appropriate
install instructions (winget / brew / apt / dnf / pacman / official installer link). NVECode
**does not bundle** any toolchain — it only detects and uses what the user already has.

## License

[MIT](./LICENSE), © Neofilisoft. NVECode is a derivative work of Microsoft's Visual
Studio Code and of VSCodium; original copyrights are preserved in `LICENSE`.

## Credits

- Microsoft and the VS Code contributors for the upstream editor.
- [VSCodium](https://github.com/VSCodium/vscodium) for the build pipeline, telemetry-removal
  patches and the FOSS extension toolchain (`@vscodium/vsce`, `@vscodium/native-keymap`,
  `@vscodium/policy-watcher`) which NVECode continues to depend on.
- [Open VSX](https://open-vsx.org/) and the Eclipse Foundation for the alternative gallery.
