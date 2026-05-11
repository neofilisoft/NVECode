# Toolchain detection

NVECode does **not** bundle compilers or runtimes. Instead, on startup the
`nve-toolchain` built-in extension probes the user's `PATH` and reports what's found
in the status bar and in the **NVECode: Toolchains** view.

| Tool       | Probe                                          | Recommended  | Install hint                                 |
|------------|------------------------------------------------|--------------|----------------------------------------------|
| Rust       | `rustc --version`                              | 1.95+        | `winget install Rustlang.Rustup` / `brew install rustup-init` / [rustup.rs](https://rustup.rs/) |
| Cargo      | `cargo --version`                              | bundled      | Comes with rustup.                            |
| Lua        | `lua -v`                                       | 5.5          | `winget install LuaBinaries.Lua` / `brew install lua` / `apt install lua5.4` |
| Python     | `python --version` / `python3 --version`       | 3.10+        | `winget install Python.Python.3.12` / `brew install python` |
| Pip        | `pip --version`                                | bundled      | Bundled with Python ≥ 3.4.                   |
| C++ (clang)| `clang++ --version`                            | C++17 / 20   | Xcode CLT / `apt install clang` / `winget install LLVM.LLVM` |
| C++ (gcc)  | `g++ --version`                                | C++17 / 20   | `apt install build-essential` / `brew install gcc` |
| C++ (msvc) | `cl.exe` on Windows                            | VS 2022      | Visual Studio Build Tools 2022.              |
| Node       | `node --version`                               | 22+          | `winget install OpenJS.NodeJS.LTS` / `brew install node` |

## Status-bar widget

A `Toolchains: 4/5` widget appears in the right-hand status bar. Clicking it opens the
**NVECode: Toolchains** view which lists each toolchain, its detected version, and a
*Copy install command* button for missing ones.

## Disabling the prompt

Set `nvecode.toolchains.checkOnStartup` to `false` if you don't want the prompt.
The view is still reachable from the command palette.

## Implementation

- Patch: [`patches/00-nve-toolchain-detect.patch`](../patches/00-nve-toolchain-detect.patch)
- Built-in extension: [`src/stable/builtin-extensions/nve-toolchain`](../src/stable/builtin-extensions/nve-toolchain)
