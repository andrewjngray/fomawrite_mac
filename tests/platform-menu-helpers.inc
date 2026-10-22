// Test-only platform boundaries. Missing production menus must fail rather
// than being replaced or activated by a fixture.
namespace {
bool testDesktopMenusMatchPlatform(QObject *window) {
    const bool mac = window->property("isMac").toBool();
    const bool windows = window->property("isWindows").toBool();
    auto *native = window->findChild<QObject *>("nativeViewMenu");
    auto *desktop = window->findChild<QObject *>("windowsMenuBar");
    auto *loader = window->findChild<QObject *>("windowsMenuLoader");
    if (mac) return !windows && native && !desktop && window->findChild<QObject *>("nativeFormatMenu");
    return windows && !native && desktop && loader && loader->property("active").toBool()
        && !window->findChild<QObject *>("nativeFormatMenu");
}
QObject *testWindowsMenuItem(QObject *window, const QString &text) {
    if (!window->property("isWindows").toBool() || !testDesktopMenusMatchPlatform(window)) return nullptr;
    auto *desktop = window->findChild<QObject *>("windowsMenuBar");
    for (auto *item : desktop->findChildren<QObject *>())
        if (item->property("text").toString() == text && item->metaObject()->indexOfSignal("triggered()") >= 0) return item;
    return nullptr;
}
QObject *testDesktopCommand(QObject *window, const QString &id) {
    if (!testDesktopMenusMatchPlatform(window)) return nullptr;
    return window->findChild<QObject *>((window->property("isMac").toBool() ? QStringLiteral("native_") : QStringLiteral("windows_")) + id);
}
bool testTriggerDesktopCommand(QObject *window, const QString &id) {
    auto *item = testDesktopCommand(window, id);
    return item && item->property("enabled").toBool() && QMetaObject::invokeMethod(item, "triggered");
}
bool testCommandState(QObject *window, const char *method, const QString &id) {
    auto *commands = window->findChild<QObject *>("workspaceCommands");
    QVariant result;
    return commands && QMetaObject::invokeMethod(commands, method, Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, id)) && result.toBool();
}
QString testCommandLabel(QObject *window, const QString &id) {
    auto *commands = window->findChild<QObject *>("workspaceCommands");
    QVariant result;
    if (!commands || !QMetaObject::invokeMethod(commands, "label", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, id))) return {};
    return result.toString();
}
bool testRunCommand(QObject *window, const QString &id) {
    auto *commands = window->findChild<QObject *>("workspaceCommands");
    return commands && QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, id));
}
}
