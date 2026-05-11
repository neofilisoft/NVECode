import type { Provider } from '../providers';

/**
 * Azure OpenAI uses deployment-scoped URLs. We treat `baseUrl` as the deployment
 * endpoint, e.g. https://my-resource.openai.azure.com/openai/deployments/my-deployment
 * and `model` as the api-version string.
 */
export const azureOpenAiProvider: Provider = {
	id: 'azure-openai',
	label: 'Azure OpenAI',
	requiresApiKey: true,
	requiresBaseUrl: true,
	defaultModel: '2024-08-01-preview',

	async chat({ baseUrl, model, systemPrompt, history, apiKey }) {
		if (!apiKey) { throw new Error('API key is required.'); }
		if (!baseUrl) { throw new Error('Deployment URL is required.'); }
		const apiVersion = model || '2024-08-01-preview';
		const messages: { role: string; content: string }[] = [];
		if (systemPrompt) { messages.push({ role: 'system', content: systemPrompt }); }
		for (const m of history) { messages.push({ role: m.role, content: m.text }); }
		const res = await fetch(`${baseUrl.replace(/\/+$/, '')}/chat/completions?api-version=${encodeURIComponent(apiVersion)}`, {
			method: 'POST',
			headers: { 'content-type': 'application/json', 'api-key': apiKey },
			body: JSON.stringify({ messages, stream: false })
		});
		if (!res.ok) { throw new Error(`HTTP ${res.status}: ${await res.text()}`); }
		const json = await res.json() as { choices?: { message?: { content?: string } }[] };
		return json.choices?.[0]?.message?.content ?? '';
	}
};
