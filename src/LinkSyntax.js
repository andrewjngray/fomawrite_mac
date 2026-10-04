.pragma library

// A bounded ordinary-inline-link grammar. Unsupported Markdown stays in Source.
function decode(value) {
    return value.replace(/\\([!"#$%&'()*+,\-./:;<=>?@\[\]\\^_`{|}~])/g, "$1");
}
function escapeLabel(value) {
    return value.replace(/\\/g, "\\\\").replace(/\[/g, "\\[").replace(/\]/g, "\\]");
}
function destination(value) {
    var escaped = value.replace(/\\/g, "\\\\").replace(/</g, "\\<").replace(/>/g, "\\>");
    return /[\s()<>]/.test(value) ? "<" + escaped + ">" : escaped;
}
function serialize(label, url, title, originalLabelSyntax) {
    var result = "[" + (originalLabelSyntax === undefined ? escapeLabel(label) : originalLabelSyntax) + "](" + destination(url);
    if (title.length) result += " \"" + title.replace(/\\/g, "\\\\").replace(/"/g, "\\\"") + "\"";
    return result + ")";
}
function touches(start, end, first, last) {
    return start === end ? start >= first && start < last : start < last && end > first;
}
function analyze(source, selectionStart, selectionEnd) {
    var start = Math.min(selectionStart, selectionEnd), end = Math.max(selectionStart, selectionEnd);
    var lineStart = source.lastIndexOf("\n", start - 1) + 1;
    var lineEnd = source.indexOf("\n", end);
    if (lineEnd < 0) lineEnd = source.length;
    var line = source.slice(lineStart, lineEnd);
    var unavailable = "This Markdown form needs Source editing. Select ordinary text or a supported inline link.";
    if (/\r|\n/.test(source.slice(start, end))) return { error: "Select text on one line to insert a link." };
    // Multiline link/reference constructs are not supported by this editor.
    // Refuse their whole span rather than treating a continuation line as prose.
    var multiline = /!?\[(?:\\.|[^\]\\])*\](?:\((?:\\.|[^)\\])*\)|\[(?:\\.|[^\]\\])*\])/g, multilineMatch;
    while ((multilineMatch = multiline.exec(source)) !== null)
        if (/[\r\n]/.test(multilineMatch[0]) && touches(start, end, multilineMatch.index, multilineMatch.index + multilineMatch[0].length))
            return { error: unavailable };
    // Do not mistake examples inside fenced or inline code for editable links.
    var prefix = source.slice(0, lineStart).split("\n"), fence = "", fenceLength = 0;
    for (var index = 0; index < prefix.length; ++index) {
        var marker = /^ {0,3}(`{3,}|~{3,})/.exec(prefix[index]);
        if (!marker) continue;
        if (!fence) { fence = marker[1].charAt(0); fenceLength = marker[1].length; }
        else if (marker[1].charAt(0) === fence && marker[1].length >= fenceLength && prefix[index].slice(marker[0].length).trim() === "") { fence = ""; fenceLength = 0; }
    }
    if (fence || /^ {0,3}(`{3,}|~{3,})/.test(line)) return { error: unavailable };
    if (/^(?: {4}|\t)/.test(line) || /^ {0,3}\[[^\]]+\]:/.test(line)) return { error: unavailable };
    var commentStart = source.lastIndexOf("<!--", start);
    if (commentStart >= 0 && source.lastIndexOf("-->", start) < commentStart) return { error: unavailable };
    var comment = /<!--[\s\S]*?(?:-->|$)/g, commentMatch;
    while ((commentMatch = comment.exec(source)) !== null)
        if (touches(start, end, commentMatch.index, commentMatch.index + commentMatch[0].length)) return { error: unavailable };
    // A nested label cannot be safely flattened into this simple-link editor.
    // Reject the containing construct before the ordinary grammar can match a
    // valid-looking substring of it. Escaped brackets are ordinary text.
    var brackets = [], nestedStart = -1;
    for (var b = 0; b < line.length; ++b) {
        if (line.charAt(b) === "\\") { ++b; continue; }
        if (line.charAt(b) === "[") {
            if (brackets.length && nestedStart < 0) nestedStart = brackets[0];
            brackets.push(b);
        } else if (line.charAt(b) === "]" && brackets.length) brackets.pop();
    }
    if (nestedStart >= 0 && end >= lineStart + nestedStart) return { error: unavailable };
    var code = /(`+)([^`]|`(?!`))*?\1/g, match;
    while ((match = code.exec(line)) !== null)
        if (touches(start, end, lineStart + match.index, lineStart + match.index + match[0].length)) return { error: unavailable };
    var pattern = /\[((?:\\.|[^\[\]\\\r\n])*)\]\((?:<((?:\\.|[^<>\\\r\n])*)>|((?:\\.|[^\s()\\\r\n])*))(?:[ \t]+(?:"((?:\\.|[^"\\\r\n])*)"|'((?:\\.|[^'\\\r\n])*)'))?[ \t]*\)/g;
    while ((match = pattern.exec(line)) !== null) {
        var first = lineStart + match.index, last = first + match[0].length;
        var before = first > 0 ? source.charAt(first - 1) : "";
        if (before === "!" || before === "[" || before === "\\") continue;
        if (start >= first && end <= last) return { existing: {
            start: first, end: last, markdown: match[0], labelSyntax: match[1], label: decode(match[1]),
            url: decode(match[2] === undefined ? match[3] : match[2]),
            title: decode(match[4] === undefined ? (match[5] || "") : match[4])
        }};
        if (touches(start, end, first, last)) return { error: unavailable };
    }
    // Images, reference/wiki links, autolinks and complex inline forms are not
    // insertion points: wrapping a substring could corrupt their delimiters.
    var protectedSyntax = /!?\[[^\r\n]*?\](?:\([^\r\n]*?\)|\[[^\]\r\n]*\])|\[\[[^\r\n]*?\]\]|<(?:[a-zA-Z][a-zA-Z0-9+.-]*:)[^>\r\n]*>/g;
    while ((match = protectedSyntax.exec(line)) !== null)
        if (touches(start, end, lineStart + match.index, lineStart + match.index + match[0].length)) return { error: unavailable };
    // A standalone bracket label may be a shortcut reference whose definition
    // lives elsewhere. Conservatively keep bracketed labels in Source rather
    // than nesting a new link in them. Email autolinks are likewise indivisible.
    var opaque = /\[(?:\\.|[^\[\]\\])*\]|<[^<>\s]+@[^<>\s]+>/g, opaqueMatch;
    while ((opaqueMatch = opaque.exec(source)) !== null)
        if (touches(start, end, opaqueMatch.index, opaqueMatch.index + opaqueMatch[0].length)) return { error: unavailable };
    return { existing: null };
}
