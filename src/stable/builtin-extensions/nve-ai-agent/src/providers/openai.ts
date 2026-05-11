import type { Provider } from '../providers';

export const openAiProvider: Provider = {
	id: 'openai',
	label: 'OpenAI',
	requiresApiKey: true,
	requiresBaseUrl: false,
	defaultBaseUrl: 'https://api.openai.com/v1',
	defaultModel: '',

	async chat({ baseUrl, model, systemPrompt, history, apiKey }) {
		if (!apiKey) { throw new Error('API key is required.'); }
		const base = baseUrl || 'https://api.openai.com/v1';
		const messages: { role: string; content: string }[] = [];
		if (systemPrompt) { messages.push({ role: 'system', content: systemPrompt }); }
		for (const m of history) { messages.push({ role: m.role, content: m.text }); }
		const res = await fetch(`${base.replace(/\/+$/, '')}/chat/completions`, {
			method: 'POST',
			headers: {
				'content-type': 'application/json',
				'authorization': `Bearer ${apiKey}`
			},
			body: JSON.stringify({ model, messages, stream: false })
		});
		if (!res.ok) { throw new Error(`HTTP ${res.status}: ${await res.text()}`); }
		const json = await res.json() as { choices?: { message?: { content?: string } }[] };
		return json.choices?.[0]?.message?.content ?? '';
	}
};
