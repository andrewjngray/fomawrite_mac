#include <QWindow>
#import <AppKit/AppKit.h>

// Keep Qt's expanded client area, but let our one toolbar own the title label.
void configureMacWindowChrome(QWindow *window) {
    if (!window) return;
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    NSWindow *native = view.window;
    native.titleVisibility = NSWindowTitleHidden;
    native.titlebarAppearsTransparent = YES;
}
