#include <QVariantMap>
#include <QGuiApplication>
#include <QScreen>
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
    // Tests use Qt's offscreen platform, which has no NSView. Keep the same
    // available-screen geometry contract there while production delegates to
    // AppKit's native centering behavior.
    if (action == "center" && QGuiApplication::platformName() != "cocoa") {
        QScreen *screen = window->screen();
        if (!screen || window->visibility() != QWindow::Windowed) return;
        const QRect available = screen->availableGeometry();
        window->setPosition(available.center() - QPoint(window->width() / 2, window->height() / 2));
        return;
    }
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    NSWindow *native = view.window;
    if (action == "minimize") [native miniaturize:nil];
    else if (action == "zoom") [native zoom:nil];
    else if (action == "center") {
        if (!native.isMiniaturized && !(native.styleMask & NSWindowStyleMaskFullScreen)) [native center];
    }
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

void applyMacWindowTheme(QWindow *window, bool followSystem, bool dark) {
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    view.window.appearance = followSystem ? nil : [NSAppearance appearanceNamed:dark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua];
}


QStringList macWritingLanguages() {
    QStringList languages;
    for(NSString *language in [[NSSpellChecker sharedSpellChecker] availableLanguages]) languages.append(QString::fromUtf8(language.UTF8String));
    languages.sort(); return languages;
}
QVariantList macWritingIssues(const QString &text,const QString &language,bool grammar) {
    NSString *input=[NSString stringWithUTF8String:text.toUtf8().constData()];
    NSSpellChecker *checker=[NSSpellChecker sharedSpellChecker];
    NSString *chosen=[NSString stringWithUTF8String:language.toUtf8().constData()];
    if(![[[NSSpellChecker sharedSpellChecker] availableLanguages] containsObject:chosen]) chosen=checker.language;
    const NSInteger tag=[NSSpellChecker uniqueSpellDocumentTag];
    QVariantList issues; NSUInteger position=0;
    while(position<input.length && issues.size()<100) {
        NSRange range=[checker checkSpellingOfString:input startingAt:position language:chosen wrap:NO inSpellDocumentWithTag:tag wordCount:nil];
        if(range.location==NSNotFound || range.location<position || !range.length) break;
        QStringList suggestions;
        for(NSString *guess in [checker guessesForWordRange:range inString:input language:chosen inSpellDocumentWithTag:tag]) {
            if(suggestions.size()>=8) break; suggestions.append(QString::fromUtf8(guess.UTF8String));
        }
        issues.append(QVariantMap{{"start",int(range.location)},{"end",int(NSMaxRange(range))},{"word",QString::fromUtf8([input substringWithRange:range].UTF8String)},
            {"label","Spelling"},{"suggestions",suggestions}});
        position=NSMaxRange(range);
    }
    if(grammar) {
        position=0;
        while(position<input.length && issues.size()<100) {
            NSArray *details=nil;
            NSRange sentence=[checker checkGrammarOfString:input startingAt:position language:chosen wrap:NO inSpellDocumentWithTag:tag details:&details];
            if(sentence.location==NSNotFound || sentence.location<position || !sentence.length) break;
            for(NSDictionary *detail in details) {
                NSRange local=[detail[NSGrammarRange] rangeValue];
                NSRange range=NSMakeRange(sentence.location+local.location,local.length);
                if(NSMaxRange(range)>input.length || !range.length || issues.size()>=100) continue;
                QStringList suggestions;
                for(NSString *guess in detail[NSGrammarCorrections]) { if(suggestions.size()>=8) break; suggestions.append(QString::fromUtf8(guess.UTF8String)); }
                NSString *description=detail[NSGrammarUserDescription];
                issues.append(QVariantMap{{"start",int(range.location)},{"end",int(NSMaxRange(range))},{"word",QString::fromUtf8([input substringWithRange:range].UTF8String)},
                    {"label",description ? QString::fromUtf8(description.UTF8String) : QString("Grammar")},{"suggestions",suggestions}});
            }
            position=NSMaxRange(sentence);
        }
    }
    [checker closeSpellDocumentWithTag:tag]; return issues;
}
static NSSpeechSynthesizer *writingSpeaker=nil;
void macSpeakText(const QString &text) {
    if(!writingSpeaker) writingSpeaker=[[NSSpeechSynthesizer alloc] initWithVoice:nil];
    [writingSpeaker stopSpeaking];
    [writingSpeaker startSpeakingString:[NSString stringWithUTF8String:text.toUtf8().constData()]];
}
void macStopSpeaking() { [writingSpeaker stopSpeaking]; }

void shareMacFile(QWindow *window, const QString &path) {
    if (!window) return;
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    static NSSharingServicePicker *picker = nil;
    [picker release];
    NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.toUtf8().constData()]];
    picker = [[NSSharingServicePicker alloc] initWithItems:@[url]];
    [picker showRelativeToRect:NSMakeRect(20, 20, 1, 1) ofView:view preferredEdge:NSMinYEdge];
}
