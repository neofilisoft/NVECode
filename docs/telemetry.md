# Telemetry & offline mode

NVECode ships with **all** upstream Microsoft Visual Studio Code telemetry endpoints
removed at build time (see `undo_telemetry.sh` and `patches/00-brand-*.patch`). What
this means in practice:

- No `aiKey` is written to `product.json`.
- No reporting URLs (`logUploaderUrl`, `surveys.feedbackUrl`, etc.) are configured.
- The `telemetry.telemetryLevel` setting still exists in the UI but has no upstream sink
  to write to.
- Built-in extensions that phone home (`ms-vscode.*` Live Share telemetry, etc.) are
  patched to be no-ops.

## Offline mode (runtime toggle)

In addition to the build-time scrubbing, NVECode exposes:

- A status-bar widget labelled **Online / Offline** in the bottom-right.
- A setting `nvecode.offlineMode` (`boolean`, default `false`).
- A command `NVECode: Toggle Offline Mode`.

When offline mode is on, NVECode:

- Blocks all requests to the extension gallery (Marketplace or Open VSX), even though
  the URLs remain configured in `product.json`.
- Disables the AI Agent's network calls (the panel still works for already-cached
  conversations but new completions fail loudly instead of silently leaking).
- Disables update checks.
- Disables remote workspace recommendations.

The toggle is implemented by `patches/00-nve-marketplace-offline.patch` adding a global
`OfflineModeService` that gateways `IRequestService` and `IUpdateService`.

## Switching the marketplace backend at build time

See [`marketplace.md`](./marketplace.md).
