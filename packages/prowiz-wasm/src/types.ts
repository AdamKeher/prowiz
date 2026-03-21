/**
 * A single format match found within the input buffer.
 */
export interface ScanEntry {
  /** Human-readable format name, e.g. "NoisePacker 3" */
  id: string;
  /** Byte offset of the match within the input buffer */
  offset: number;
  /** Size of the match in bytes */
  size: number;
  /** Sequential index (0-based) among all matches in this scan */
  index: number;
  /** true = music module; false = data cruncher / executable */
  isModule: boolean;
  /** true = the data extends beyond the end of the input buffer */
  truncated: boolean;
}

export interface ScanResult {
  entries: ScanEntry[];
  /** Total bytes scanned */
  inputSize: number;
}

export interface ExtractResult {
  /** Raw bytes of the extracted entry (may be packed, not depacked) */
  data: Uint8Array;
  entry: ScanEntry;
}

export interface ProwizOptions {
  /** Include music module hits (default: true) */
  includeModules?: boolean;
  /** Include data cruncher / executable hits (default: false) */
  includeData?: boolean;
}

export interface Prowiz {
  /**
   * Scan the input buffer and return all format matches.
   */
  scan(input: Uint8Array, options?: ProwizOptions): Promise<ScanResult>;

  /**
   * Extract the raw bytes for the match at `index`.
   * For packed formats this returns the packed data, not the depacked module.
   */
  extract(input: Uint8Array, index: number, options?: ProwizOptions): Promise<ExtractResult>;

  /**
   * Convenience: return the first module match, or null if none found.
   */
  identify(input: Uint8Array): Promise<ScanEntry | null>;
}

/**
 * Initialise the WASM module and return a Prowiz instance.
 */
export declare function createProwiz(): Promise<Prowiz>;
