#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include "resizable_item.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <QPointer>

class ImageItem : public ResizableItem
{
    Q_OBJECT

public:
    explicit ImageItem(const QString &imagePath = "", Workspace *parent = nullptr);

    QString type() const override;

    QJsonObject serialize() const override;
    void deserialize(const QJsonObject &json) override;

private:
    QPointer<QLabel> _imageLabel;
    QString _imagePath;
};

#endif // IMAGEITEM_H
