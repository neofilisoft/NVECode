/**
 * Provider adapter registry. Adding a new provider is a matter of dropping a new
 * `*-provider.ts` file here and exporting it from `listProviders()`. No part of the
 * workbench is aware of any specific provider name — everything is wired through
 * settings.
 */

import { openAiCompatibleProvider } from './providers/openai-compatible';
import { anthropicProvider } from './providers/anthropic';
import { ollamaProvider } from './providers/ollama';
import { openAiProvider } from './providers/openai';
import { azureOpenAiProvider } from './providers/azure-openai';

export type ProviderId =
	| 'openai-compatible'
	| 'anthropic'
	| 'ollama'
	| 'openai'
	| 'azure-openai';

export interface ProviderMeta {
	id: ProviderId;
	label: string;
	requiresApiKey: boolean;
	requiresBaseUrl: boolean;
	defaultBaseUrl?: string;
	defaultModel?: string;
}

export interface ChatRequest {
	baseUrl: string;
	model: string;
	systemPrompt: string;
	history: { role: 'user' | 'assistant'; text: string }[];
	apiKey?: string;
}

export interface Provider extends ProviderMeta {
	chat(req: ChatRequest): Promise<string>;
}

const providers: Record<ProviderId, Provider> = {
	'openai-compatible': openAiCompatibleProvider,
	'anthropic':         anthropicProvider,
	'ollama':            ollamaProvider,
	'openai':            openAiProvider,
	'azure-openai':      azureOpenAiProvider
};

export function getProvider(id: ProviderId): Provider {
	return providers[id] ?? providers['openai-compatible'];
}

export function listProviders(): Provider[] {
	return Object.values(providers);
}
