/**
 * index.ts — public API for @prowiz/wasm
 *
 * Usage:
 *   import { createProwiz } from '@prowiz/wasm';
 *
 *   const prowiz = await createProwiz();
 *   const result = await prowiz.scan(fileBytes, { includeModules: true });
 *   console.log(result.entries);
 */

export type { ScanEntry, ScanResult, ExtractResult, ProwizOptions, Prowiz } from './types.js';

import { loadWasm, type WasmModule } from './loader.js';
import type { ScanEntry, ScanResult, ExtractResult, ProwizOptions, Prowiz } from './types.js';

function parseJson(raw: string): ScanEntry[] {
  const parsed = JSON.parse(raw) as {
    results: Array<{
      id: string;
      offset: number;
      size: number;
      index: number;
      is_module: boolean;
      truncated: boolean;
    }>;
  };
  return parsed.results.map((r) => ({
    id: r.id,
    offset: r.offset,
    size: r.size,
    index: r.index,
    isModule: r.is_module,
    truncated: r.truncated,
  }));
}

function copyToWasm(wasm: WasmModule, data: Uint8Array): number {
  const ptr = wasm._malloc(data.byteLength);
  wasm.HEAPU8.set(data, ptr);
  return ptr;
}

class ProwizImpl implements Prowiz {
  constructor(private wasm: WasmModule) {}

  async scan(input: Uint8Array, options: ProwizOptions = {}): Promise<ScanResult> {
    const includeModules = options.includeModules !== false ? 1 : 0;
    const includeData = options.includeData ? 1 : 0;

    const ptr = copyToWasm(this.wasm, input);
    try {
      const jsonPtr: number = this.wasm.ccall(
        'prowiz_scan',
        'number',
        ['number', 'number', 'number', 'number'],
        [ptr, input.byteLength, includeModules, includeData],
      );
      if (jsonPtr === 0) throw new Error('prowiz_scan returned null');
      const json = this.wasm.UTF8ToString(jsonPtr);
      this.wasm._free(jsonPtr);
      return { entries: parseJson(json), inputSize: input.byteLength };
    } finally {
      this.wasm._free(ptr);
    }
  }

  async extract(input: Uint8Array, index: number, _options: ProwizOptions = {}): Promise<ExtractResult> {
    // First scan to find the entry metadata
    const scanResult = await this.scan(input, { includeModules: true, includeData: true });
    const entry = scanResult.entries.find((e) => e.index === index);
    if (!entry) throw new Error(`No entry at index ${index}`);
    if (entry.truncated) throw new Error(`Entry at index ${index} is truncated`);

    const inPtr = copyToWasm(this.wasm, input);
    const outPtr = this.wasm._malloc(entry.size);
    try {
      const written: number = this.wasm.ccall(
        'prowiz_extract',
        'number',
        ['number', 'number', 'number', 'number'],
        [inPtr, input.byteLength, index, outPtr],
      );
      if (written < 0) throw new Error(`prowiz_extract failed for index ${index}`);
      const data = this.wasm.HEAPU8.slice(outPtr, outPtr + written);
      return { data, entry };
    } finally {
      this.wasm._free(inPtr);
      this.wasm._free(outPtr);
    }
  }

  async identify(input: Uint8Array): Promise<ScanEntry | null> {
    const result = await this.scan(input, { includeModules: true, includeData: false });
    return result.entries.find((e) => e.isModule) ?? null;
  }
}

export async function createProwiz(): Promise<Prowiz> {
  const wasm = await loadWasm();
  return new ProwizImpl(wasm);
}
