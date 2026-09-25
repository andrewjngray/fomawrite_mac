#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <QString>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (argc != 4)
        return 2;
    bool ok = false;
    const int size = QString::fromLocal8Bit(argv[3]).toInt(&ok);
    if (!ok || size < 16 || size > 1024)
        return 2;
    QSvgRenderer svg(QString::fromLocal8Bit(argv[1]));
    if (!svg.isValid())
        return 3;
    QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    svg.render(&painter);
    painter.end();
    return image.save(QString::fromLocal8Bit(argv[2]), "PNG") ? 0 : 4;
}
