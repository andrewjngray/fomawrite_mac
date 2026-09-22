#include <QVariantMap>
#include <QWindow>
#import <AppKit/AppKit.h>

// Keep Qt's expanded client area, but let our one toolbar own the title label.
void configureMacWindowChrome(QWindow *window) {
    if (!window) return;
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    NSWindow *native = view.window;
    // Request the roomier native unified title bar. AppKit owns control size,
    // placement and corner treatment; Qt continues to draw the toolbar items.
    native.tabbingIdentifier = @"OmawriteDocuments";
    native.tabbingMode = NSWindowTabbingModeDisallowed;
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

#include <QString>
#include <QStringList>

void performMacWindowAction(QWindow *window, const QString &action, const QString &text) {
    if (!window) return;
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    NSWindow *native = view.window;
    if (action == "minimize") [native miniaturize:nil];
    else if (action == "zoom") [native zoom:nil];
    else if (action == "front") [NSApp arrangeInFront:nil];
    else if (action == "merge") {
        for (NSWindow *other in NSApp.windows) {
            if (other != native && [other.tabbingIdentifier isEqualToString:@"OmawriteDocuments"] && other.isVisible) {
                native.tabbingMode = NSWindowTabbingModePreferred;
                other.tabbingMode = NSWindowTabbingModePreferred;
                [native addTabbedWindow:other ordered:NSWindowAbove];
            }
        }
    }
    else if (action == "nextTab") [native selectNextTab:nil];
    else if (action == "previousTab") [native selectPreviousTab:nil];
    else if (action == "detach") [native moveTabToNewWindow:nil];
    else if (action == "tabBar") [native toggleTabBar:nil];
    else if (action == "overview") [native toggleTabOverview:nil];
    else if (action == "emoji") [NSApp orderFrontCharacterPalette:nil];
    else if (action == "share") {
        static NSSharingServicePicker *picker = nil;
        [picker release];
        NSString *content = [NSString stringWithUTF8String:text.toUtf8().constData()];
        picker = [[NSSharingServicePicker alloc] initWithItems:@[content]];
        [picker showRelativeToRect:NSMakeRect(20, 20, 1, 1) ofView:view preferredEdge:NSMinYEdge];
    }
}

QStringList macSpellingIssues(const QString &text) {
    NSString *input = [NSString stringWithUTF8String:text.toUtf8().constData()];
    NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
    QStringList issues;
    NSUInteger position = 0;
    while (position < input.length && issues.size() < 100) {
        NSRange range = [checker checkSpellingOfString:input startingAt:position];
        if (range.location == NSNotFound || range.location < position || range.length == 0) break;
        NSString *word = [input substringWithRange:range];
        issues.append(QString::fromUtf8(word.UTF8String));
        position = NSMaxRange(range);
    }
    issues.removeDuplicates();
    return issues;
}

#include <QVariantList>
#include <QDateTime>
#include <QUrl>

QString createMacVersion(const QString &path) {
    NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.toUtf8().constData()]];
    NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
    __block NSError *versionError = nil;
    NSError *coordinationError = nil;
    [coordinator coordinateWritingItemAtURL:url options:0 error:&coordinationError byAccessor:^(NSURL *coordinatedURL) {
        [NSFileVersion addVersionOfItemAtURL:coordinatedURL withContentsOfURL:coordinatedURL options:0 error:&versionError];
    }];
    NSError *error = coordinationError ?: versionError;
    QString result = error ? QString::fromUtf8(error.localizedDescription.UTF8String) : QString();
    [coordinator release];
    return result;
}
QVariantList macVersions(const QString &path) {
    NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.toUtf8().constData()]];
    QVariantList versions;
    for (NSFileVersion *version in [NSFileVersion otherVersionsOfItemAtURL:url]) {
        versions.append(QVariantMap{{"url", QUrl::fromLocalFile(QString::fromUtf8(version.URL.path.UTF8String))},
            {"date", QDateTime::fromMSecsSinceEpoch([version.modificationDate timeIntervalSince1970] * 1000).toString(Qt::ISODate)}});
    }
    return versions;
}

QVariantList macWordClasses(const QString &text) {
    NSString *input = [NSString stringWithUTF8String:text.toUtf8().constData()];
    NSLinguisticTagger *tagger = [[NSLinguisticTagger alloc] initWithTagSchemes:@[NSLinguisticTagSchemeLexicalClass] options:0];
    tagger.string = input;
    __block QVariantList result;
    [tagger enumerateTagsInRange:NSMakeRange(0, input.length) scheme:NSLinguisticTagSchemeLexicalClass
        options:NSLinguisticTaggerOmitWhitespace | NSLinguisticTaggerOmitPunctuation
        usingBlock:^(NSLinguisticTag tag, NSRange tokenRange, NSRange, BOOL *stop) {
            if (result.size() >= 500) { *stop = YES; return; }
            result.append(QVariantMap{{"start", int(tokenRange.location)}, {"end", int(NSMaxRange(tokenRange))},
                {"label", QString::fromUtf8(tag.UTF8String)}, {"word", QString::fromUtf8([input substringWithRange:tokenRange].UTF8String)}});
        }];
    [tagger release];
    return result;
}

int macTabInset(QWindow *window) {
    if (!window) return 0;
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    NSWindow *native = view.window;
    if (!native.tabGroup.isTabBarVisible) return 0;
    return qMax(0, qRound(native.frame.size.height - native.contentLayoutRect.size.height) - 44);
}

// Keep AppKit tab inspection and restoration out of the portable session store.
QVariantMap macWorkspaceState(QWindow *window) {
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    NSWindow *native = view.window;
    NSArray<NSWindow *> *tabs = native.tabbedWindows;
    if (tabs.count < 2) return {};
    return {{"group", QString::number(tabs.firstObject.windowNumber)},
            {"order", int([tabs indexOfObject:native])}};
}
void restoreMacWorkspaceTabs(const QList<QWindow *> &windows) {
    if (windows.size() < 2) return;
    NSWindow *previous = nil;
    for (auto *window : windows) {
        NSView *view = reinterpret_cast<NSView *>(window->winId());
        NSWindow *native = view.window;
        native.tabbingMode = NSWindowTabbingModePreferred;
        if (previous) [previous addTabbedWindow:native ordered:NSWindowAbove];
        previous = native;
    }
}
