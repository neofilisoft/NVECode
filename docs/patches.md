# Patches

Custom logic on top of upstream `vscode` lives in [`patches/`](../patches). Every file
in this directory whose name ends in `.patch` is applied in lexical order. Sub-directories
exist for OS-specific patches:

- `patches/linux/*.patch`
- `patches/windows/*.patch`
- `patches/osx/*.patch`
- `patches/insider/*.patch`
- `patches/user/*.patch`  &nbsp;←&nbsp; for downstream forks; ignored on clean clones.

## Placeholders

`apply_patch` replaces these tokens before applying the patch:

| Token                       | Filled from |
|-----------------------------|-------------|
| `!!APP_NAME!!`              | `APP_NAME` (default `NVECode`)        |
| `!!APP_NAME_LC!!`           | lowercase of `APP_NAME`               |
| `!!BINARY_NAME!!`           | `BINARY_NAME` (default `vecode`)      |
| `!!ORG_NAME!!`              | `ORG_NAME` (default `Neofilisoft`)    |
| `!!PUBLISHER!!`             | `PUBLISHER`                           |
| `!!PRODUCT_LONG_NAME!!`     | `PRODUCT_LONG_NAME`                   |
| `!!ASSETS_REPOSITORY!!`     | `ASSETS_REPOSITORY`                   |
| `!!GH_REPO_PATH!!`          | `GH_REPO_PATH`                        |
| `!!TUNNEL_APP_NAME!!`       | `TUNNEL_APP_NAME`                     |

Always prefer placeholders over hard-coded brand strings. The build will fail to apply
a patch if any of its `+` lines mention `VSCodium` / `vscodium` / `codium` outside of
legitimate `@vscodium/*` npm package paths.

## Categories

| Prefix          | Meaning                                                 |
|-----------------|---------------------------------------------------------|
| `00-nve-*`      | NVECode-specific feature patches (AI Agent, auth, …).   |
| `00-brand-*`    | Branding / telemetry removal.                           |
| `00-build-*`    | Build pipeline tweaks (sourcemap URLs, update server).  |
| `00-ui-*`       | Cosmetic / theme tweaks.                                |
| `20-*` / `21-*` | Replace proprietary MS libs with FOSS forks            (`@vscodium/native-keymap`, `@vscodium/policy-watcher`, …). |

## Regenerating a patch

```bash
cd vscode/
# edit files
git diff --no-color > ../patches/00-nve-my-change.patch
```

Then re-run the full pipeline (`./dev/build.sh`) to confirm it still applies cleanly.
