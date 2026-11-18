#import <AppKit/AppKit.h>
#include <QStringList>

// Top-level items of the real macOS menu bar, as the menu titles the user
// sees (the submenu title), marking hidden items.
QStringList nativeMenuTitles() {
    QStringList titles;
    for (NSMenuItem *item in [[NSApp mainMenu] itemArray]) {
        NSString *title = item.submenu.title.length ? item.submenu.title : item.title;
        titles << QString::fromNSString(title) + (item.hidden ? QStringLiteral("(hidden)") : QString());
    }
    return titles;
}
