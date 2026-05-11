import type { Provider } from '../providers';

/**
 * Generic OpenAI Chat Completions adapter. Works against any endpoint that speaks
 * /v1/chat/completions — vLLM, LM Studio, llama.cpp server, LocalAI, OpenRouter, etc.
 * The base URL must include the /v1 prefix.
 */
export const openAiCompatibleProvider: Provider = {
	id: 'openai-compatible',
	label: 'OpenAI-Compatible Endpoint',
	requiresApiKey: false,
	requiresBaseUrl: true,
	defaultBaseUrl: '',
	defaultModel: '',

	async chat({ baseUrl, model, systemPrompt, history, apiKey }) {
		if (!baseUrl) { throw new Error('Base URL is required.'); }
		if (!model)   { throw new Error('Model id is required.'); }
		const messages = [];
		if (systemPrompt) { messages.push({ role: 'system', content: systemPrompt }); }
		for (const m of history) {
			messages.push({ role: m.role, content: m.text });
		}
		const res = await fetch(`${baseUrl.replace(/\/+$/, '')}/chat/completions`, {
			method: 'POST',
			headers: {
				'content-type': 'application/json',
				...(apiKey ? { 'authorization': `Bearer ${apiKey}` } : {})
			},
			body: JSON.stringify({ model, messages, stream: false })
		});
		if (!res.ok) { throw new Error(`HTTP ${res.status}: ${await res.text()}`); }
		const json = await res.json() as { choices?: { message?: { content?: string } }[] };
		return json.choices?.[0]?.message?.content ?? '';
	}
};
