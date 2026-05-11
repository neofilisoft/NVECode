import type { Provider } from '../providers';

export const ollamaProvider: Provider = {
	id: 'ollama',
	label: 'Ollama (local)',
	requiresApiKey: false,
	requiresBaseUrl: true,
	defaultBaseUrl: 'http://localhost:11434',
	defaultModel: 'llama3.1',

	async chat({ baseUrl, model, systemPrompt, history }) {
		const base = (baseUrl || 'http://localhost:11434').replace(/\/+$/, '');
		const messages: { role: string; content: string }[] = [];
		if (systemPrompt) { messages.push({ role: 'system', content: systemPrompt }); }
		for (const m of history) { messages.push({ role: m.role, content: m.text }); }
		const res = await fetch(`${base}/api/chat`, {
			method: 'POST',
			headers: { 'content-type': 'application/json' },
			body: JSON.stringify({ model, messages, stream: false })
		});
		if (!res.ok) { throw new Error(`HTTP ${res.status}: ${await res.text()}`); }
		const json = await res.json() as { message?: { content?: string } };
		return json.message?.content ?? '';
	}
};
