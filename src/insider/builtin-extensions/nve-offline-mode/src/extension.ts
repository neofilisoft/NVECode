import * as vscode from 'vscode';

let statusBar: vscode.StatusBarItem;

export function activate(context: vscode.ExtensionContext) {
	statusBar = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 90);
	statusBar.command = 'nve.offlineMode.toggle';
	context.subscriptions.push(
		statusBar,
		vscode.commands.registerCommand('nve.offlineMode.toggle', () => toggle()),
		vscode.commands.registerCommand('nve.offlineMode.on',     () => setOffline(true)),
		vscode.commands.registerCommand('nve.offlineMode.off',    () => setOffline(false)),
		vscode.workspace.onDidChangeConfiguration(e => {
			if (e.affectsConfiguration('nvecode.offlineMode')) { refresh(); }
		})
	);
	refresh();
}

export function deactivate() {
	statusBar?.dispose();
}

async function toggle(): Promise<void> {
	const cfg = vscode.workspace.getConfiguration('nvecode');
	const current = cfg.get<boolean>('offlineMode', false);
	await setOffline(!current);
}

async function setOffline(value: boolean): Promise<void> {
	const cfg = vscode.workspace.getConfiguration('nvecode');
	await cfg.update('offlineMode', value, vscode.ConfigurationTarget.Global);
	refresh();
	vscode.window.showInformationMessage(value
		? 'NVECode is now offline. Marketplace, updates and the AI Agent are paused.'
		: 'NVECode is back online.');
}

function refresh(): void {
	const offline = vscode.workspace.getConfiguration('nvecode').get<boolean>('offlineMode', false);
	statusBar.text = offline ? '$(cloud-offline) Offline' : '$(cloud) Online';
	statusBar.tooltip = offline
		? 'NVECode is offline. Click to go back online.'
		: 'NVECode is online. Click to go offline.';
	statusBar.show();
	// Workbench code reads this on the global object to make routing decisions.
	(globalThis as { __nveOfflineMode?: boolean }).__nveOfflineMode = offline;
}
