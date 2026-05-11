# Authentication

NVECode supports four sign-in modes. None of the OAuth client IDs are baked into the
build — they are read from environment variables, the user settings file, or the system
secret store at runtime.

## Modes

| Mode    | Notes                                                                 |
|---------|-----------------------------------------------------------------------|
| Google  | OAuth 2.0 device-code flow. Requires `NVE_GOOGLE_OAUTH_CLIENT_ID`.    |
| GitHub  | OAuth web flow. Requires `NVE_GITHUB_OAUTH_CLIENT_ID`.                |
| GitLab  | OAuth web flow against gitlab.com or a self-managed instance. Requires `NVE_GITLAB_OAUTH_CLIENT_ID` and optionally `NVE_GITLAB_BASE_URL` (default `https://gitlab.com`). |
| Guest   | No network call. A local-only profile is created so the user can still personalize the editor and use the AI Agent. |

## How NVECode finds the client IDs

In order of precedence:

1. Environment variable (e.g. `NVE_GITHUB_OAUTH_CLIENT_ID`).
2. User settings file (`nvecode.auth.github.clientId`).
3. The `nve.auth` section of `product.json` (organization deployments — see below).

If none is set, that provider is greyed-out in the sign-in picker.

## Organization deployments

To pre-configure OAuth client IDs for an entire enterprise rollout, drop a
`product.overrides.json` next to `vecode.exe` (or `vecode` on POSIX) containing:

```json
{
  "nve": {
    "auth": {
      "google":  { "clientId": "…apps.googleusercontent.com" },
      "github":  { "clientId": "Iv1.…" },
      "gitlab":  { "clientId": "abc…", "baseUrl": "https://gitlab.example.com" }
    }
  }
}
```

This file is merged into the in-memory `product` object at startup. **Client secrets
are never read from this file** — the PKCE flow doesn't need one.

## Implementation

- Patch: [`patches/00-nve-auth.patch`](../patches/00-nve-auth.patch)
- Built-in extension: [`src/stable/builtin-extensions/nve-auth`](../src/stable/builtin-extensions/nve-auth)
