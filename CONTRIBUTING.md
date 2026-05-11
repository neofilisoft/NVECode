# Contributing to NVECode

Thanks for considering a contribution.

## Reporting bugs

Open issues at <https://github.com/Neofilisoft/nvecode/issues>. Please separate:

- **NVECode-specific bugs** — anything related to the AI Agent, sign-in flow, marketplace
  switcher, toolchain detection, or the `vecode` binary / installer.
- **Upstream bugs** — issues that also reproduce in upstream Microsoft Visual Studio Code
  belong at <https://github.com/microsoft/vscode/issues>.

Include:

- NVECode version (`vecode --version`).
- OS and architecture.
- Marketplace mode (online / offline) and whether telemetry is enabled.
- Minimal reproduction steps.

## Building from source

See [`docs/howto-build.md`](docs/howto-build.md).

## Pull requests

- Branch from `main`.
- Keep changes focused; large refactors should be discussed in an issue first.
- Run the patches end-to-end (`./dev/build.sh -s -o`) to confirm they still apply.
- Do **not** hard-code provider names, API keys, OAuth client IDs, or remote URLs in
  patches. Use environment variables and `product.json` defaults instead.

## Patch authoring

NVECode uses unified `.patch` files (the same format as VSCodium). Patches in
`patches/` are applied in lexical order on top of upstream `vscode/`. Filenames are
prefixed by category:

| Prefix       | Meaning                                       |
|--------------|-----------------------------------------------|
| `00-nve-*`   | NVECode-specific feature patches.             |
| `00-brand-*` | Branding / telemetry removal.                 |
| `00-build-*` | Build pipeline tweaks.                        |
| `00-ui-*`    | Cosmetic / theme patches.                     |
| `20-*` / `21-*` | Replace proprietary MS libs with FOSS forks. |

Placeholders like `!!APP_NAME!!`, `!!BINARY_NAME!!`, `!!ASSETS_REPOSITORY!!` are
substituted at apply time from `utils.sh`. Prefer them over hard-coded brand strings.

## Code of conduct

By participating you agree to follow the [Code of Conduct](CODE_OF_CONDUCT.md).
