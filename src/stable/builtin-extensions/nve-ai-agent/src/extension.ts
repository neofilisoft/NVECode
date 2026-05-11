import * as vscode from 'vscode';
import { ProviderId, getProvider, listProviders } from './providers';

const SECRET_KEY_PREFIX = 'nvecode.aiAgent.apiKey.';

export function activate(context: vscode.ExtensionContext) {
	const provider = new ChatViewProvider(context);
	context.subscriptions.push(
		vscode.window.registerWebviewViewProvider('nve-ai-agent.chat', provider, {
			webviewOptions: { retainContextWhenHidden: true }
		}),
		vscode.commands.registerCommand('nve.aiAgent.open', () =>
			vscode.commands.executeCommand('workbench.view.extension.nve-ai-agent')),
		vscode.commands.registerCommand('nve.aiAgent.newChat', () => provider.newChat()),
		vscode.commands.registerCommand('nve.aiAgent.configure', () => configure(context)),
		vscode.commands.registerCommand('nve.aiAgent.clearSecrets', () => clearAllSecrets(context))
	);
}

export function deactivate() { /* noop */ }

async function configure(context: vscode.ExtensionContext): Promise<void> {
	const providers = listProviders();
	const picked = await vscode.window.showQuickPick(
		providers.map(p => ({ label: p.label, description: p.id, id: p.id })),
		{ placeHolder: 'Choose an LLM provider — bring your own.' }
	);
	if (!picked) { return; }
	const cfg = vscode.workspace.getConfiguration('nvecode.aiAgent');
	await cfg.update('provider', picked.id, vscode.ConfigurationTarget.Global);
	const meta = providers.find(p => p.id === picked.id)!;
	if (meta.requiresBaseUrl) {
		const baseUrl = await vscode.window.showInputBox({
			prompt: `Base URL for ${meta.label}`,
			value: cfg.get<string>('baseUrl') || meta.defaultBaseUrl || '',
			ignoreFocusOut: true
		});
		if (baseUrl !== undefined) {
			await cfg.update('baseUrl', baseUrl, vscode.ConfigurationTarget.Global);
		}
	}
	if (meta.requiresApiKey) {
		const apiKey = await vscode.window.showInputBox({
			prompt: `API key for ${meta.label} (stored in OS secret store)`,
			password: true,
			ignoreFocusOut: true
		});
		if (apiKey) {
			await context.secrets.store(SECRET_KEY_PREFIX + meta.id, apiKey);
		}
	}
	const model = await vscode.window.showInputBox({
		prompt: 'Model id',
		value: cfg.get<string>('model') || meta.defaultModel || '',
		ignoreFocusOut: true
	});
	if (model !== undefined) {
		await cfg.update('model', model, vscode.ConfigurationTarget.Global);
	}
	vscode.window.showInformationMessage('NVECode AI Agent configured.');
}

async function clearAllSecrets(context: vscode.ExtensionContext): Promise<void> {
	for (const p of listProviders()) {
		await context.secrets.delete(SECRET_KEY_PREFIX + p.id);
	}
	vscode.window.showInformationMessage('NVECode AI Agent credentials cleared.');
}

class ChatViewProvider implements vscode.WebviewViewProvider {
	private view: vscode.WebviewView | undefined;
	private history: { role: 'user' | 'assistant'; text: string }[] = [];

	constructor(private readonly context: vscode.ExtensionContext) {}

	resolveWebviewView(webviewView: vscode.WebviewView): void {
		this.view = webviewView;
		webviewView.webview.options = { enableScripts: true };
		webviewView.webview.html = this.renderHtml();
		webviewView.webview.onDidReceiveMessage(async msg => {
			if (msg.type === 'send') {
				await this.send(msg.text as string);
			} else if (msg.type === 'configure') {
				await vscode.commands.executeCommand('nve.aiAgent.configure');
			} else if (msg.type === 'clear') {
				this.history = [];
				this.post({ type: 'state', history: this.history });
			}
		});
		this.post({ type: 'state', history: this.history });
	}

	newChat() {
		this.history = [];
		this.post({ type: 'state', history: this.history });
	}

	private async send(text: string): Promise<void> {
		if (!text.trim()) { return; }
		const cfg = vscode.workspace.getConfiguration('nvecode.aiAgent');
		if (!cfg.get<boolean>('enabled', true)) {
			this.post({ type: 'error', text: 'AI Agent is disabled in settings.' });
			return;
		}
		const offline = vscode.workspace.getConfiguration('nvecode').get<boolean>('offlineMode', false);
		if (offline) {
			this.post({ type: 'error', text: 'NVECode is in offline mode. Toggle it off to use the AI Agent.' });
			return;
		}
		const providerId = cfg.get<string>('provider', 'openai-compatible') as ProviderId;
		const adapter = getProvider(providerId);
		const apiKey = await this.context.secrets.get(SECRET_KEY_PREFIX + providerId);
		this.history.push({ role: 'user', text });
		this.post({ type: 'state', history: this.history });
		try {
			const reply = await adapter.chat({
				baseUrl: cfg.get<string>('baseUrl', ''),
				model: cfg.get<string>('model', ''),
				systemPrompt: cfg.get<string>('systemPrompt', ''),
				history: this.history,
				apiKey
			});
			this.history.push({ role: 'assistant', text: reply });
		} catch (err) {
			this.history.push({ role: 'assistant', text: `Error: ${String(err)}` });
		}
		this.post({ type: 'state', history: this.history });
	}

	private post(msg: object): void {
		this.view?.webview.postMessage(msg);
	}

	private renderHtml(): string {
		return /* html */ `<!DOCTYPE html>
<html><head><meta charset="utf-8"><meta http-equiv="Content-Security-Policy"
content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';"/>
<style>
body { font-family: var(--vscode-font-family); padding: 8px; color: var(--vscode-foreground); }
#log { display: flex; flex-direction: column; gap: 8px; margin-bottom: 8px; }
.msg { padding: 8px; border-radius: 4px; white-space: pre-wrap; }
.user { background: var(--vscode-editor-selectionBackground); }
.assistant { background: var(--vscode-textBlockQuote-background); }
textarea { width: 100%; box-sizing: border-box; min-height: 60px; }
.row { display: flex; gap: 4px; margin-top: 4px; }
button { padding: 4px 8px; }
</style></head>
<body>
<div id="log"></div>
<textarea id="input" placeholder="Ask the AI Agent…"></textarea>
<div class="row">
  <button id="send">Send</button>
  <button id="configure">Configure provider</button>
  <button id="clear">Clear chat</button>
</div>
<script>
const vscode = acquireVsCodeApi();
const log = document.getElementById('log');
const input = document.getElementById('input');
document.getElementById('send').onclick = () => {
  vscode.postMessage({ type: 'send', text: input.value });
  input.value = '';
};
document.getElementById('configure').onclick = () => vscode.postMessage({ type: 'configure' });
document.getElementById('clear').onclick = () => vscode.postMessage({ type: 'clear' });
window.addEventListener('message', e => {
  const msg = e.data;
  if (msg.type === 'state') {
    log.innerHTML = '';
    for (const m of msg.history) {
      const div = document.createElement('div');
      div.className = 'msg ' + m.role;
      div.textContent = m.text;
      log.appendChild(div);
    }
  } else if (msg.type === 'error') {
    const div = document.createElement('div');
    div.className = 'msg assistant';
    div.textContent = msg.text;
    log.appendChild(div);
  }
});
</script>
</body></html>`;
	}
}
