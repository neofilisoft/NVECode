import * as vscode from 'vscode';
import { execFile } from 'child_process';

interface ToolchainProbe {
	id: string;
	label: string;
	command: string;
	args: string[];
	versionRegex: RegExp;
	install: Record<NodeJS.Platform, string>;
}

const IS_WIN  = process.platform === 'win32';
const IS_MAC  = process.platform === 'darwin';

const PROBES: ToolchainProbe[] = [
	{
		id: 'rust', label: 'Rust',
		command: 'rustc', args: ['--version'],
		versionRegex: /rustc\s+(\d+\.\d+\.\d+)/,
		install: {
			win32:  'winget install Rustlang.Rustup',
			darwin: 'brew install rustup-init && rustup-init -y',
			linux:  'curl --proto =https --tlsv1.2 -sSf https://sh.rustup.rs | sh',
		} as Record<NodeJS.Platform, string>
	},
	{
		id: 'cargo', label: 'Cargo',
		command: 'cargo', args: ['--version'],
		versionRegex: /cargo\s+(\d+\.\d+\.\d+)/,
		install: {
			win32:  'Comes with rustup',
			darwin: 'Comes with rustup',
			linux:  'Comes with rustup'
		} as Record<NodeJS.Platform, string>
	},
	{
		id: 'lua', label: 'Lua',
		command: 'lua', args: ['-v'],
		versionRegex: /Lua\s+(\d+\.\d+(?:\.\d+)?)/,
		install: {
			win32:  'winget install LuaBinaries.Lua',
			darwin: 'brew install lua',
			linux:  'sudo apt install -y lua5.4   # or build 5.5 from source'
		} as Record<NodeJS.Platform, string>
	},
	{
		id: 'python', label: 'Python',
		command: IS_WIN ? 'python' : 'python3', args: ['--version'],
		versionRegex: /Python\s+(\d+\.\d+\.\d+)/,
		install: {
			win32:  'winget install Python.Python.3.12',
			darwin: 'brew install python',
			linux:  'sudo apt install -y python3'
		} as Record<NodeJS.Platform, string>
	},
	{
		id: 'cxx-clang', label: 'C++ (clang)',
		command: 'clang++', args: ['--version'],
		versionRegex: /version\s+(\d+\.\d+\.\d+)/,
		install: {
			win32:  'winget install LLVM.LLVM',
			darwin: 'xcode-select --install',
			linux:  'sudo apt install -y clang'
		} as Record<NodeJS.Platform, string>
	},
	{
		id: 'cxx-gcc', label: 'C++ (g++)',
		command: 'g++', args: ['--version'],
		versionRegex: /\b(\d+\.\d+\.\d+)\b/,
		install: {
			win32:  'winget install MSYS2.MSYS2   # then pacman -S mingw-w64-x86_64-gcc',
			darwin: 'brew install gcc',
			linux:  'sudo apt install -y build-essential'
		} as Record<NodeJS.Platform, string>
	},
];

if (IS_WIN) {
	PROBES.push({
		id: 'cxx-msvc', label: 'C++ (MSVC)',
		command: 'cl', args: ['/?'],
		versionRegex: /Version\s+(\d+\.\d+\.\d+)/,
		install: { win32: 'Install Visual Studio Build Tools 2022', darwin: '', linux: '' } as Record<NodeJS.Platform, string>
	});
}

interface ProbeResult {
	probe: ToolchainProbe;
	version?: string;
	error?: string;
}

let lastResults: ProbeResult[] = [];
let statusBar: vscode.StatusBarItem;

export async function activate(context: vscode.ExtensionContext) {
	statusBar = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 80);
	statusBar.command = 'nve.toolchains.show';
	context.subscriptions.push(statusBar);

	const tree = new ToolchainTree();
	context.subscriptions.push(vscode.window.registerTreeDataProvider('nve-toolchain.list', tree));

	context.subscriptions.push(
		vscode.commands.registerCommand('nve.toolchains.show', () =>
			vscode.commands.executeCommand('workbench.view.extension.nve-toolchain')),
		vscode.commands.registerCommand('nve.toolchains.recheck', () => check(tree))
	);

	if (vscode.workspace.getConfiguration().get<boolean>('nvecode.toolchains.checkOnStartup', true)) {
		await check(tree);
	}
}

export function deactivate() { statusBar?.dispose(); }

async function check(tree: ToolchainTree): Promise<void> {
	lastResults = await Promise.all(PROBES.map(probeOne));
	const found = lastResults.filter(r => r.version).length;
	statusBar.text = `$(tools) Toolchains: ${found}/${lastResults.length}`;
	statusBar.tooltip = lastResults.map(r => r.version
		? `${r.probe.label}: ${r.version}`
		: `${r.probe.label}: missing`).join('\n');
	statusBar.show();
	tree.refresh(lastResults);
}

function probeOne(probe: ToolchainProbe): Promise<ProbeResult> {
	return new Promise(resolve => {
		execFile(probe.command, probe.args, { timeout: 4000 }, (err, stdout, stderr) => {
			if (err) {
				resolve({ probe, error: err.message });
				return;
			}
			const haystack = `${stdout}\n${stderr}`;
			const m = probe.versionRegex.exec(haystack);
			resolve({ probe, version: m?.[1] });
		});
	});
}

class ToolchainTree implements vscode.TreeDataProvider<ProbeResult> {
	private _onDidChange = new vscode.EventEmitter<void>();
	readonly onDidChangeTreeData = this._onDidChange.event;

	refresh(results: ProbeResult[]): void {
		lastResults = results;
		this._onDidChange.fire();
	}

	getTreeItem(element: ProbeResult): vscode.TreeItem {
		const item = new vscode.TreeItem(element.probe.label);
		if (element.version) {
			item.description = `v${element.version}`;
			item.iconPath = new vscode.ThemeIcon('pass');
			item.tooltip = `${element.probe.command} ${element.probe.args.join(' ')}\n${element.version}`;
		} else {
			item.description = 'not found';
			item.iconPath = new vscode.ThemeIcon('warning');
			const hint = element.probe.install[process.platform] ?? '';
			item.tooltip = `Install hint: ${hint}`;
			item.command = {
				command: 'workbench.action.terminal.sendSequence',
				title: 'Copy install command',
				arguments: [{ text: hint }]
			};
		}
		return item;
	}

	getChildren(): ProbeResult[] {
		return lastResults;
	}
}
