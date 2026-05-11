# Marketplace

NVECode supports three marketplace backends, chosen at build time via the
`MARKETPLACE_BACKEND` env var (see [`prepare_vscode.sh`](../prepare_vscode.sh)):

| Value                       | Gallery                                  |
|-----------------------------|------------------------------------------|
| `vscode-marketplace` (default) | Microsoft Visual Studio Marketplace — the same backend Cursor and Windsurf use. |
| `open-vsx`                  | [Eclipse Open VSX](https://open-vsx.org/) — the default VSCodium gallery. |
| `offline` / `none`          | No remote gallery. Only `code --install-extension *.vsix` works. |

```bash
MARKETPLACE_BACKEND=open-vsx ./dev/build.sh
```

Regardless of which backend is baked in, the runtime **Offline Mode** switch
(`nvecode.offlineMode`) gateways every gallery request — see
[`telemetry.md`](./telemetry.md).

## Licensing note

The Microsoft Visual Studio Marketplace Terms of Use restrict who may consume the
gallery API. Read the [terms](https://aka.ms/vsmarketplace-tou) before shipping
production builds with `vscode-marketplace` enabled. Open VSX has no equivalent
restriction.
