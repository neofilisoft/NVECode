# AI Agent

NVECode ships a sidebar AI Agent that is **pluggable**: nothing about the provider,
endpoint, or model is baked into the binary.

## Configuring a provider

1. Open the AI Agent view (`Ctrl+Alt+A` / `Cmd+Alt+A`).
2. On first run NVECode prompts for:
   - **Provider** — one of `openai-compatible`, `anthropic`, `ollama`, `openai`, `azure-openai`.
   - **Base URL** — only for `openai-compatible`, `ollama`, `azure-openai`.
   - **API key** — only if the provider needs one. Stored in the OS secret store
     (`Keychain` on macOS, `Credential Vault` on Windows, `libsecret` on Linux).
   - **Model id** — free-form string (e.g. `gpt-4o-mini`, `claude-3.5-sonnet`,
     `llama3.1:70b-instruct-q4_K_M`).
3. The configuration is persisted to the user settings file under `nvecode.aiAgent.*`.
   Secrets stay in the OS vault and are referenced by id.

## Settings keys

| Setting                              | Type      | Notes |
|--------------------------------------|-----------|-------|
| `nvecode.aiAgent.enabled`            | boolean   | Master switch. Defaults to `true`. |
| `nvecode.aiAgent.provider`           | enum      | `"openai-compatible"`, `"anthropic"`, `"ollama"`, `"openai"`, `"azure-openai"`. |
| `nvecode.aiAgent.baseUrl`            | string    | Used for openai-compatible / ollama / azure-openai. |
| `nvecode.aiAgent.model`              | string    | Model id. |
| `nvecode.aiAgent.contextWindowChars` | number    | Rolling context size. Default 32000. |
| `nvecode.aiAgent.allowToolUse`       | boolean   | Whether the agent can invoke local tools (read/write file, run task). |
| `nvecode.aiAgent.systemPrompt`       | string    | Optional override. |

## Offline mode

When `nvecode.offlineMode` is on, the agent refuses to send requests and shows an
inline "Offline" banner. Cached conversations remain readable. See
[`telemetry.md`](./telemetry.md).

## Implementation

- Patch: [`patches/00-nve-ai-agent.patch`](../patches/00-nve-ai-agent.patch)
- Built-in extension: [`src/stable/builtin-extensions/nve-ai-agent`](../src/stable/builtin-extensions/nve-ai-agent)
- The provider adapter pattern means new providers can be added without touching
  workbench code — drop a new `*-provider.ts` into the built-in extension.
