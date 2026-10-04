// Linked only by bin/test-window-routing, never by a shipped application.
#include <QWindow>
#include <QDebug>
#import <AppKit/AppKit.h>

void smokeActivateApplication(QWindow *window) {
    // A command-line-launched GUI can have focused Qt items while its whole
    // application remains inactive. Real WindowShortcut dispatch requires both.
    static bool reported = false;
    const NSInteger previousPolicy = [NSApp activationPolicy];
    const bool policySet = [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    const bool requested = [[NSRunningApplication currentApplication] activateWithOptions:NSApplicationActivateIgnoringOtherApps];
    [NSApp activate];
    if (!reported) {
        qInfo() << "NATIVE ACTIVATION:" << "previousPolicy" << previousPolicy
                << "regularPolicyAccepted" << policySet << "requestAccepted" << requested
                << "nativeActive" << bool([NSApp isActive]);
        reported = true;
    }
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    [view.window makeKeyAndOrderFront:nil];
    window->raise();
    window->requestActivate();
}
