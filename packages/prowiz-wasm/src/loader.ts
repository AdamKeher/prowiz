/**
 * loader.ts — environment-aware WASM loader
 *
 * Works in both Node.js (via fs.readFile) and the browser (via fetch).
 * The Emscripten glue (prowiz.js) is loaded first; it in turn fetches
 * prowiz.wasm from the same directory.
 */

/* eslint-disable @typescript-eslint/no-explicit-any */

declare function require(id: string): any;
declare const __dirname: string;

export interface WasmModule {
  ccall(name: string, returnType: string, argTypes: string[], args: any[]): any;
  _malloc(size: number): number;
  _free(ptr: number): void;
  HEAPU8: Uint8Array;
  UTF8ToString(ptr: number): string;
}

let cachedModule: WasmModule | null = null;

export async function loadWasm(): Promise<WasmModule> {
  if (cachedModule) return cachedModule;

  // Detect environment
  const isNode =
    typeof process !== 'undefined' &&
    process.versions != null &&
    process.versions.node != null;

  let ProwizWasmFactory: (opts?: object) => Promise<WasmModule>;

  if (isNode) {
    // Node.js: require the emscripten glue directly
    // eslint-disable-next-line @typescript-eslint/no-var-requires
    ProwizWasmFactory = require('../wasm/prowiz.js');
  } else {
    // Browser: dynamic import (bundler must handle the wasm asset)
    const mod = await import('../wasm/prowiz.js' as string);
    ProwizWasmFactory = mod.default ?? mod;
  }

  const module = await ProwizWasmFactory();
  cachedModule = module;
  return module;
}
