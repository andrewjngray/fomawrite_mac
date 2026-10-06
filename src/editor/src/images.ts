// Paste / drop of images (Typora behaviour): the bytes go to the host (`bridge.saveImage`), which saves them
// next to the document and answers with `bridge.imageSaved(token, relativePath, error)`; only then is the
// Markdown image inserted, at the position remembered when the paste/drop happened (mapped through any edits
// made in the meantime). Text pastes are never touched. See README "Pasting images".
import { ChangeSet, EditorSelection, EditorState, Extension, StateEffect, StateField, TransactionSpec } from "@codemirror/state";
import { EditorView } from "@codemirror/view";

export const MAX_IMAGE_BYTES = 20 * 1024 * 1024;
export const IMAGE_MIME_TYPES = ["image/png", "image/jpeg", "image/gif", "image/webp"] as const;
const EXTENSIONS: Record<string, string> = { "image/png": "png", "image/jpeg": "jpg", "image/gif": "gif", "image/webp": "webp" };

// ---------------------------------------------------------------- validation (pure)

/** Error message for a MIME type / byte size the editor will not send to the host, or null when acceptable. */
export function validateImage(mime: string, size: number): string | null {
  if (!(IMAGE_MIME_TYPES as readonly string[]).includes(mime)) return `unsupported image type "${mime}" (png, jpeg, gif, webp only)`;
  if (!(size > 0)) return "image is empty";
  if (size > MAX_IMAGE_BYTES) return `image is too large (${(size / 1048576).toFixed(1)} MiB, limit ${MAX_IMAGE_BYTES / 1048576} MiB)`;
  return null;
}

/** Decoded byte length of a canonical base64 string, or -1 when it is not canonical base64. */
export function base64ByteLength(b64: string): number {
  if (b64.length === 0 || b64.length % 4 !== 0 || !/^[A-Za-z0-9+/]*={0,2}$/.test(b64)) return -1;
  const pad = b64.endsWith("==") ? 2 : b64.endsWith("=") ? 1 : 0;
  return (b64.length / 4) * 3 - pad;
}

/** Full check of what is about to be sent: MIME, well-formed base64, decoded size. Null when acceptable. */
export function validateImagePayload(mime: string, base64: string): string | null {
  const bytes = base64ByteLength(base64);
  if (bytes < 0) return "image data is not valid base64";
  return validateImage(mime, bytes);
}

// ---------------------------------------------------------------- names and Markdown text (pure)

const pad2 = (n: number) => String(n).padStart(2, "0");

/** File name suggested to the host: the pasted/dropped file's own name (sanitised, extension ensured), else `pasted-image-YYYYMMDD-HHMMSS.ext`. */
export function suggestFileName(fileName: string | undefined, mime: string, now: Date = new Date()): string {
  const ext = EXTENSIONS[mime] ?? "png";
  let base = (fileName ?? "").split(/[\\/]/).pop()!.replace(/[\u0000-\u001f\u007f]/g, "").trim();
  if (base === "" || base === "." || base === "..") {
    const stamp = `${now.getFullYear()}${pad2(now.getMonth() + 1)}${pad2(now.getDate())}-${pad2(now.getHours())}${pad2(now.getMinutes())}${pad2(now.getSeconds())}`;
    return `pasted-image-${stamp}.${ext}`;
  }
  if (!/\.[A-Za-z0-9]{2,5}$/.test(base)) base += "." + ext;
  return base;
}

/** `%`, whitespace and parentheses would end or corrupt the link destination: percent-encode them. */
export function encodeImagePath(path: string): string {
  return path.replace(/[%()\s]/g, (c) => (c.charCodeAt(0) < 128 ? "%" + c.charCodeAt(0).toString(16).toUpperCase().padStart(2, "0") : encodeURIComponent(c)));
}

/** Alt text: the file stem of the path (last segment without its extension), with `\`, `[` and `]` escaped. */
export function imageAlt(relativePath: string): string {
  const last = relativePath.split("/").pop() ?? "";
  const dot = last.lastIndexOf(".");
  const stem = (dot > 0 ? last.slice(0, dot) : last).replace(/[\r\n]+/g, " ").trim() || "image";
  return stem.replace(/[\\[\]]/g, (c) => "\\" + c);
}

/** `![alt](relative%20path.png)`. */
export function imageMarkdown(relativePath: string): string {
  return `![${imageAlt(relativePath)}](${encodeImagePath(relativePath)})`;
}

// ---------------------------------------------------------------- pending insertions (pure over EditorState)

/** A save in flight. `batch`/`index`/`count` order the images of one multi-file drop: one image per line, in file order. */
export interface Pending { pos: number; batch: number; index: number; count: number }
export type PendingMap = ReadonlyMap<number, Pending>;

export const addPendingEffect = StateEffect.define<{ token: number; pending: Pending }>();
/** The save is finished (inserted or failed): forget it. `len` = length of text inserted at its position (0 on failure). */
export const settleEffect = StateEffect.define<{ token: number; len: number }>();

/** Carry every pending position through a document change (`assoc -1`: text typed at the position later goes after the image). */
export function mapPending(pending: PendingMap, changes: ChangeSet): Map<number, Pending> {
  const out = new Map<number, Pending>();
  for (const [token, p] of pending) out.set(token, { ...p, pos: changes.mapPos(p.pos, -1) });
  return out;
}

/** Remove `token` and push the not-yet-inserted later images of its batch (still at its position) past the `len` inserted characters. */
export function settlePending(pending: Map<number, Pending>, token: number, len: number): void {
  const done = pending.get(token);
  if (!done) return;
  pending.delete(token);
  if (len <= 0) return;
  for (const [t, p] of pending) if (p.batch === done.batch && p.index > done.index && p.pos === done.pos) pending.set(t, { ...p, pos: p.pos + len });
}

export const pendingField = StateField.define<PendingMap>({
  create: () => new Map(),
  update(value, tr) {
    let next: Map<number, Pending> | null = tr.docChanged ? mapPending(value, tr.changes) : null;
    for (const e of tr.effects) {
      if (e.is(addPendingEffect)) (next ??= new Map(value)).set(e.value.token, e.value.pending);
      else if (e.is(settleEffect)) settlePending((next ??= new Map(value)), e.value.token, e.value.len);
    }
    return next ?? value;
  },
});

/** The transaction that inserts the saved image for `token` (null when the token is unknown, e.g. the document was replaced meanwhile). */
export function insertionSpec(state: EditorState, token: number, relativePath: string): TransactionSpec | null {
  const p = state.field(pendingField, false)?.get(token);
  if (!p) return null;
  const text = imageMarkdown(relativePath) + (p.index < p.count - 1 ? "\n" : "");
  const pos = Math.min(p.pos, state.doc.length);
  const sel = state.selection.main;
  return {
    changes: { from: pos, insert: text },
    effects: settleEffect.of({ token, len: text.length }),
    // A caret still sitting at the paste/drop point follows the new image; anything else keeps its place.
    selection: sel.empty && sel.head === pos ? EditorSelection.cursor(pos + text.length) : undefined,
    userEvent: "input.image",
  };
}

// ---------------------------------------------------------------- the extension

interface ImageBridge {
  log?: (message: string) => void;
  saveImage?: (token: number, name: string, mime: string, base64: string) => void;
  imageSaved?: { connect(fn: (token: number, relativePath: string, error: string) => void): void };
}

function readBase64(file: File): Promise<string> {
  return new Promise((resolve, reject) => {
    const r = new FileReader();
    r.onerror = () => reject(r.error ?? new Error("could not read the image"));
    r.onload = () => {
      const s = String(r.result);
      resolve(s.slice(s.indexOf(",") + 1));
    };
    r.readAsDataURL(file);
  });
}

function imageFiles(list: Iterable<File> | null | undefined): File[] {
  return Array.from(list ?? []).filter((f) => f.type.startsWith("image/"));
}

/** Image files on the clipboard (items first, which covers screenshot pastes; `files` for Finder copies). */
function clipboardImages(data: DataTransfer | null): File[] {
  if (!data) return [];
  const fromItems: File[] = [];
  for (const item of Array.from(data.items ?? [])) {
    if (item.kind !== "file" || !item.type.startsWith("image/")) continue;
    const f = item.getAsFile();
    if (f) fromItems.push(f);
  }
  return fromItems.length ? fromItems : imageFiles(data.files);
}

export function imagesExtension(bridge: ImageBridge): Extension {
  const log = (m: string) => { try { bridge.log?.(m); } catch { /* ignore */ } };
  let nextToken = 1, nextBatch = 1;
  let view: EditorView | null = null;

  bridge.imageSaved?.connect?.((token, relativePath, error) => {
    if (error) log(`image not saved: ${error}`);
    else if (typeof relativePath !== "string" || relativePath === "") log("image not saved: the host returned no path");
    if (!view) return;
    const spec = !error && relativePath ? insertionSpec(view.state, token, relativePath) : null;
    // No insertion (error, or the token is stale after setDocument): just forget a still-known token.
    if (spec) view.dispatch(spec);
    else if (view.state.field(pendingField, false)?.has(token)) view.dispatch({ effects: settleEffect.of({ token, len: 0 }) });
  });

  function start(v: EditorView, files: File[], pos: number): boolean {
    if (typeof bridge.saveImage !== "function") return false; // old host: keep the browser's default
    view = v;
    const accepted: File[] = [];
    for (const f of files) {
      const err = validateImage(f.type.toLowerCase(), f.size);
      if (err) log(`image ignored (${f.name || "pasted image"}): ${err}`);
      else accepted.push(f);
    }
    if (!accepted.length) return true;
    const batch = nextBatch++;
    const tokens = accepted.map(() => nextToken++);
    v.dispatch({ effects: tokens.map((token, index) => addPendingEffect.of({ token, pending: { pos, batch, index, count: accepted.length } })) });
    accepted.forEach((file, i) => {
      const token = tokens[i], mime = file.type.toLowerCase();
      const fail = (m: string) => {
        log(`image ignored (${file.name || "pasted image"}): ${m}`);
        if (view?.state.field(pendingField, false)?.has(token)) view.dispatch({ effects: settleEffect.of({ token, len: 0 }) });
      };
      readBase64(file).then((b64) => {
        const err = validateImagePayload(mime, b64);
        if (err) return fail(err);
        try { bridge.saveImage!(token, suggestFileName(file.name, mime), mime, b64); } catch (e) { fail(e instanceof Error ? e.message : String(e)); }
      }, (e) => fail(e instanceof Error ? e.message : String(e)));
    });
    return true;
  }

  return [
    pendingField,
    EditorView.domEventHandlers({
      paste(event, v) {
        const files = clipboardImages(event.clipboardData);
        if (!files.length || !start(v, files, v.state.selection.main.head)) return false;
        event.preventDefault();
        return true;
      },
      dragover(event) {
        if (!event.dataTransfer?.types?.includes("Files") || typeof bridge.saveImage !== "function") return false;
        event.preventDefault();
        return true;
      },
      drop(event, v) {
        const files = imageFiles(event.dataTransfer?.files);
        if (!files.length) return false;
        const at = v.posAtCoords({ x: event.clientX, y: event.clientY }) ?? v.state.selection.main.head;
        if (!start(v, files, at)) return false;
        event.preventDefault();
        return true;
      },
    }),
  ];
}
