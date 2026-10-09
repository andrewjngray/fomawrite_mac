#pragma once

// Character geometry for the Source editor on the accessibility bridge.
//
// Qt Quick's accessible text item reports an editable text area with its
// text, cursor and selection, but answers every character-bounds query with
// an empty rectangle and every point-to-offset query with -1 (upstream
// stubs). macOS writing assistants such as Grammarly Desktop read those
// answers as AXBoundsForRange and AXRangeForPosition, and without them they
// cannot place a button or an underline, so they never attach. This factory
// gives the Source editor (and only it) real answers from the text item's own
// layout. Install it once, before any window exists.

class QAccessibleInterface;
class QObject;
class QString;

QAccessibleInterface *sourceEditorAccessibleFactory(const QString &className, QObject *object);
void installSourceEditorAccessibility();
