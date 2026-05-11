import type { Provider } from '../providers';

export const anthropicProvider: Provider = {
	id: 'anthropic',
	label: 'Anthropic',
	requiresApiKey: true,
	requiresBaseUrl: false,
	defaultBaseUrl: 'https://api.anthropic.com',
	defaultModel: '',

	async chat({ baseUrl, model, systemPrompt, history, apiKey }) {
		if (!apiKey) { throw new Error('API key is required.'); }
		const base = baseUrl || 'https://api.anthropic.com';
		const messages = history.map(m => ({ role: m.role, content: m.text }));
		const res = await fetch(`${base.replace(/\/+$/, '')}/v1/messages`, {
			method: 'POST',
			headers: {
				'content-type': 'application/json',
				'x-api-key': apiKey,
				'anthropic-version': '2023-06-01'
			},
			body: JSON.stringify({
				model,
				max_tokens: 4096,
				system: systemPrompt || undefined,
				messages
			})
		});
		if (!res.ok) { throw new Error(`HTTP ${res.status}: ${await res.text()}`); }
		const json = await res.json() as { content?: { type: string; text?: string }[] };
		return (json.content ?? []).filter(c => c.type === 'text').map(c => c.text ?? '').join('');
	}
};
