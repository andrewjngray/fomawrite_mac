#include <QWindow>
#import <AppKit/AppKit.h>

// Keep Qt's expanded client area, but let our one toolbar own the title label.
void configureMacWindowChrome(QWindow *window) {
    if (!window) return;
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    NSWindow *native = view.window;
    // Request the roomier native unified title bar. AppKit owns control size,
    // placement and corner treatment; Qt continues to draw the toolbar items.
    native.toolbarStyle = NSWindowToolbarStyleUnified;
    if (!native.toolbar) {
        NSToolbar *toolbar = [[NSToolbar alloc] initWithIdentifier:@"OmawriteWindowToolbar"];
        toolbar.allowsUserCustomization = NO;
        toolbar.displayMode = NSToolbarDisplayModeIconOnly;
        native.toolbar = toolbar;
        [toolbar release];
    }
    native.titleVisibility = NSWindowTitleHidden;
    native.titlebarAppearsTransparent = YES;
}
