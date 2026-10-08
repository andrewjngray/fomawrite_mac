// Answer to the host's `requestSelection(token)`: the page's live main
// selection, read from the editor state at the moment of the request. The
// debounced `cursorChanged` report can lag a selection change by 30 ms, so a
// Format command asks for the selection instead of trusting the last report.
import type { EditorState } from "@codemirror/state";

export interface SelectionReplyBridge {
  selectionReply(token: number, anchor: number, head: number): void;
}

export function replySelection(
  bridge: SelectionReplyBridge,
  view: { state: Pick<EditorState, "selection"> },
  token: number,
): void {
  const sel = view.state.selection.main;
  bridge.selectionReply(token, sel.anchor, sel.head);
}
