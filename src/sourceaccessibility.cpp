#include "sourceaccessibility.h"

#include <QAccessible>
#include <QMetaObject>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRectF>
#include <QtQuick/private/qaccessiblequickitem_p.h>

namespace {

class SourceEditorAccessible : public QAccessibleQuickItem, public QAccessibleEditableTextInterface {
public:
    explicit SourceEditorAccessible(QQuickItem *item) : QAccessibleQuickItem(item) {}

    // The stock item offers an editable-text interface; keep it so an
    // assistant can write a correction back (AXValue, AXSelectedText).
    void *interface_cast(QAccessible::InterfaceType type) override {
        if (type == QAccessible::EditableTextInterface) return static_cast<QAccessibleEditableTextInterface *>(this);
        return QAccessibleQuickItem::interface_cast(type);
    }
    void deleteText(int start, int end) override { replaceText(start, end, QString()); }
    void insertText(int offset, const QString &text) override { replaceText(offset, offset, text); }
    void replaceText(int start, int end, const QString &text) override {
        QQuickItem *item = this->item();
        if (!item || start < 0 || end < start) return;
        const int count = item->property("length").toInt();
        if (end > count) return;
        if (end > start) QMetaObject::invokeMethod(item, "remove", Q_ARG(int, start), Q_ARG(int, end));
        if (!text.isEmpty()) QMetaObject::invokeMethod(item, "insert", Q_ARG(int, start), Q_ARG(QString, text));
    }

    // Screen rectangle of the character at `offset`: the caret rectangle at
    // that position widened to the caret rectangle of the next position on
    // the same line. Line ends and the last character keep the caret width.
    QRect characterRect(int offset) const override {
        QQuickItem *text = item();
        if (!text || !text->window() || offset < 0) return {};
        const int count = text->property("length").toInt();
        if (offset > count) return {};
        QRectF here;
        if (!QMetaObject::invokeMethod(text, "positionToRectangle", Q_RETURN_ARG(QRectF, here), Q_ARG(int, offset))) return {};
        if (offset < count) {
            QRectF next;
            if (QMetaObject::invokeMethod(text, "positionToRectangle", Q_RETURN_ARG(QRectF, next), Q_ARG(int, offset + 1))
                && qAbs(next.y() - here.y()) < 0.5 && next.x() > here.x())
                here.setRight(next.x());
        }
        if (here.width() < 1) here.setWidth(1);
        if (here.height() < 1) here.setHeight(1);
        const QPointF topLeft = text->mapToGlobal(here.topLeft());
        return QRectF(topLeft, here.size()).toAlignedRect();
    }

    // Text offset under a screen point, or -1 outside the item.
    int offsetAtPoint(const QPoint &point) const override {
        QQuickItem *text = item();
        if (!text || !text->window()) return -1;
        const QPointF local = text->mapFromGlobal(QPointF(point));
        if (!text->contains(local)) return -1;
        int offset = -1;
        if (!QMetaObject::invokeMethod(text, "positionAt", Q_RETURN_ARG(int, offset), Q_ARG(qreal, local.x()), Q_ARG(qreal, local.y()))) return -1;
        return offset;
    }
};

} // namespace

QAccessibleInterface *sourceEditorAccessibleFactory(const QString &className, QObject *object) {
    Q_UNUSED(className);
    if (!object || object->objectName() != QLatin1String("sourceEditor") || !object->inherits("QQuickTextEdit")) return nullptr;
    return new SourceEditorAccessible(qobject_cast<QQuickItem *>(object));
}

void installSourceEditorAccessibility() {
    static bool installed = false;
    if (installed) return;
    installed = true;
    QAccessible::installFactory(sourceEditorAccessibleFactory);
}
