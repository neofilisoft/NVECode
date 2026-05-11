import * as vscode from 'vscode';

type ProviderId = 'nve-google' | 'nve-github' | 'nve-gitlab';

interface ProviderConfig {
	id: ProviderId;
	label: string;
	settingsKey: string;
	envVar: string;
	productKey: string;
	authorizeUrl(clientId: string, baseUrl?: string): string;
	tokenUrl(baseUrl?: string): string;
	userInfoUrl(baseUrl?: string): string;
}

const PROVIDERS: ProviderConfig[] = [
	{
		id: 'nve-google',
		label: 'Google',
		settingsKey: 'nvecode.auth.google.clientId',
		envVar: 'NVE_GOOGLE_OAUTH_CLIENT_ID',
		productKey: 'google',
		authorizeUrl: (clientId) => `https://accounts.google.com/o/oauth2/v2/auth?client_id=${encodeURIComponent(clientId)}&scope=openid%20email%20profile&response_type=code&redirect_uri=${encodeURIComponent(redirectUri())}`,
		tokenUrl: () => 'https://oauth2.googleapis.com/token',
		userInfoUrl: () => 'https://openidconnect.googleapis.com/v1/userinfo'
	},
	{
		id: 'nve-github',
		label: 'GitHub',
		settingsKey: 'nvecode.auth.github.clientId',
		envVar: 'NVE_GITHUB_OAUTH_CLIENT_ID',
		productKey: 'github',
		authorizeUrl: (clientId) => `https://github.com/login/oauth/authorize?client_id=${encodeURIComponent(clientId)}&scope=read:user%20user:email&redirect_uri=${encodeURIComponent(redirectUri())}`,
		tokenUrl: () => 'https://github.com/login/oauth/access_token',
		userInfoUrl: () => 'https://api.github.com/user'
	},
	{
		id: 'nve-gitlab',
		label: 'GitLab',
		settingsKey: 'nvecode.auth.gitlab.clientId',
		envVar: 'NVE_GITLAB_OAUTH_CLIENT_ID',
		productKey: 'gitlab',
		authorizeUrl: (clientId, baseUrl) => `${baseUrl ?? 'https://gitlab.com'}/oauth/authorize?client_id=${encodeURIComponent(clientId)}&response_type=code&scope=read_user&redirect_uri=${encodeURIComponent(redirectUri())}`,
		tokenUrl: (baseUrl) => `${baseUrl ?? 'https://gitlab.com'}/oauth/token`,
		userInfoUrl: (baseUrl) => `${baseUrl ?? 'https://gitlab.com'}/api/v4/user`
	}
];

function redirectUri(): string {
	return `${vscode.env.uriScheme}://neofilisoft.nve-auth/oauth-callback`;
}

function readClientId(provider: ProviderConfig): string | undefined {
	const cfg = vscode.workspace.getConfiguration();
	const fromSettings = cfg.get<string>(provider.settingsKey);
	if (fromSettings) { return fromSettings; }
	const fromEnv = process.env[provider.envVar];
	if (fromEnv) { return fromEnv; }
	// product.overrides.json is merged into product.json at startup and exposed as a
	// global. We look it up via the workbench `product` if available.
	const productOverrides = (globalThis as { __nveProduct?: { nve?: { auth?: Record<string, { clientId?: string }> } } }).__nveProduct;
	return productOverrides?.nve?.auth?.[provider.productKey]?.clientId;
}

export function activate(context: vscode.ExtensionContext) {
	context.subscriptions.push(
		vscode.commands.registerCommand('nve.auth.signIn',  () => pickAndSignIn(context)),
		vscode.commands.registerCommand('nve.auth.signOut', () => signOut(context)),
		vscode.commands.registerCommand('nve.auth.continueAsGuest', () => continueAsGuest(context))
	);

	for (const provider of PROVIDERS) {
		context.subscriptions.push(
			vscode.authentication.registerAuthenticationProvider(
				provider.id,
				provider.label,
				new NveAuthProvider(provider, context),
				{ supportsMultipleAccounts: false }
			)
		);
	}
}

export function deactivate() { /* noop */ }

async function pickAndSignIn(context: vscode.ExtensionContext): Promise<void> {
	const items: vscode.QuickPickItem[] = PROVIDERS
		.filter(p => !!readClientId(p))
		.map(p => ({ label: `Sign in with ${p.label}`, description: p.id }));
	items.push({ label: 'Continue as Guest', description: 'no network call' });
	const picked = await vscode.window.showQuickPick(items, { placeHolder: 'How would you like to sign in?' });
	if (!picked) { return; }
	if (picked.label.startsWith('Continue as Guest')) {
		return continueAsGuest(context);
	}
	const provider = PROVIDERS.find(p => `Sign in with ${p.label}` === picked.label);
	if (!provider) { return; }
	await vscode.authentication.getSession(provider.id, [], { createIfNone: true });
}

async function signOut(context: vscode.ExtensionContext): Promise<void> {
	for (const p of PROVIDERS) {
		await context.secrets.delete(`nve.auth.${p.id}.token`);
	}
	await context.globalState.update('nve.auth.guest', false);
	vscode.window.showInformationMessage('Signed out of NVECode.');
}

async function continueAsGuest(context: vscode.ExtensionContext): Promise<void> {
	const name = vscode.workspace.getConfiguration().get<string>('nvecode.auth.guestProfileName', 'Guest');
	await context.globalState.update('nve.auth.guest', true);
	vscode.window.showInformationMessage(`Continuing as ${name}. NVECode will not call any auth endpoint.`);
}

class NveAuthProvider implements vscode.AuthenticationProvider {
	private _onDidChange = new vscode.EventEmitter<vscode.AuthenticationProviderAuthenticationSessionsChangeEvent>();
	readonly onDidChangeSessions = this._onDidChange.event;

	constructor(private readonly provider: ProviderConfig, private readonly context: vscode.ExtensionContext) {}

	async getSessions(): Promise<readonly vscode.AuthenticationSession[]> {
		const token = await this.context.secrets.get(`nve.auth.${this.provider.id}.token`);
		if (!token) { return []; }
		return [this.toSession(token)];
	}

	async createSession(): Promise<vscode.AuthenticationSession> {
		const clientId = readClientId(this.provider);
		if (!clientId) {
			throw new Error(`No client ID configured for ${this.provider.label}. Set the env var ${this.provider.envVar} or the setting ${this.provider.settingsKey}.`);
		}
		const baseUrl = this.provider.id === 'nve-gitlab'
			? vscode.workspace.getConfiguration().get<string>('nvecode.auth.gitlab.baseUrl', 'https://gitlab.com')
			: undefined;
		const url = vscode.Uri.parse(this.provider.authorizeUrl(clientId, baseUrl));
		await vscode.env.openExternal(url);
		const token = await vscode.window.showInputBox({
			prompt: `Paste the OAuth code from ${this.provider.label} (the redirect URI shows it).`,
			ignoreFocusOut: true,
			password: true
		});
		if (!token) { throw new Error('No code provided.'); }
		// The actual code↔token exchange and userinfo fetch should happen via the
		// provider-specific token endpoint; we keep the integration boundary thin
		// here so an organization can plug in PKCE / refresh-token rotation policy.
		await this.context.secrets.store(`nve.auth.${this.provider.id}.token`, token);
		const session = this.toSession(token);
		this._onDidChange.fire({ added: [session], removed: [], changed: [] });
		return session;
	}

	async removeSession(sessionId: string): Promise<void> {
		await this.context.secrets.delete(`nve.auth.${this.provider.id}.token`);
		this._onDidChange.fire({ added: [], removed: [{ id: sessionId, accessToken: '', account: { id: '', label: '' }, scopes: [] }], changed: [] });
	}

	private toSession(token: string): vscode.AuthenticationSession {
		return {
			id: this.provider.id,
			accessToken: token,
			account: { id: this.provider.id, label: this.provider.label },
			scopes: []
		};
	}
}
