#include "workspace.h"
#include "checkbox_item.h"
#include "file_item.h"
#include "image_item.h"
#include "list_item.h"
#include "nested_workspace_item.h"
#include "text_item.h"
#include "title_item.h"

#include <QScrollArea>
#include <QMenu>
#include <QToolButton>
#include <QIcon>
#include <QFileDialog>

Workspace::Workspace(const QString &name, QWidget *parent) : QWidget(parent), _workspaceName(name)
{
    QVBoxLayout *contentLayout = new QVBoxLayout(this);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    _titleLabel = new QLabel(_workspaceName, this);
    _titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = _titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    _titleLabel->setFont(titleFont);
    contentLayout->addWidget(_titleLabel);

    QToolButton *menuButton = new QToolButton(this);
    menuButton->setIcon(QIcon::fromTheme("menu"));
    menuButton->setStyleSheet("border: none;");
    menuButton->setToolTip("Добавить элемент");

    QMenu *toolMenu = new QMenu(this);
    toolMenu->addAction(QIcon::fromTheme("text"), "Добавить текст", this, [this]() {
        emit addItemByType("TextItem");
    });
    toolMenu->addAction(QIcon::fromTheme("checkbox"), "Добавить checkbox", this, [this]() {
        emit addItemByType("CheckboxItem");
    });
    toolMenu->addAction(QIcon::fromTheme("list"), "Добавить список", this, [this]() {
        emit addItemByType("ListItem");
    });
    toolMenu->addAction(QIcon::fromTheme("image"), "Добавить изображение", this, [this]() {
        emit addItemByType("ImageItem");
    });
    toolMenu->addAction(QIcon::fromTheme("file"), "Добавить файл", this, [this]() {
        emit addItemByType("FileItem");
    });

    menuButton->setMenu(toolMenu);
    menuButton->setPopupMode(QToolButton::InstantPopup);

    headerLayout->addWidget(_titleLabel);
    headerLayout->addWidget(menuButton);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    contentLayout->addLayout(headerLayout);

    _contentWidget = new QWidget(this);
    _layout = new QVBoxLayout(_contentWidget);
    _contentWidget->setLayout(_layout);

    _scrollArea = new QScrollArea(this);
    _scrollArea->setWidget(_contentWidget);
    _scrollArea->setWidgetResizable(true);
    _scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    contentLayout->addWidget(_scrollArea);
    setLayout(contentLayout);
}

void Workspace::addItemByType(const QString &type)
{
    AbstractWorkspaceItem *item = nullptr;

    if (type == "TextItem") {
        item = new TextItem("Новый текст", this);
    } else if (type == "CheckboxItem") {
        item = new CheckboxItem("Новый checkbox", this);
    } else if (type == "ListItem") {
        ListItem *list = new ListItem(ListItem::Unordered, this);
        list->addItemToList("Первый элемент");
        list->addItemToList("Второй элемент");
        item = list;
    } else if (type == "ImageItem") {
        QString imagePath = QFileDialog::getOpenFileName(this, "Выберите изображение", "",
                                                         "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
        if (!imagePath.isEmpty()) {
            item = new ImageItem(imagePath, this);
        }
    } else if (type == "FileItem") {
        QString filePath =
         QFileDialog::getOpenFileName(this, "Выберите файл", "", "All Files (*.*)");
        if (!filePath.isEmpty()) {
            item = new FileItem(filePath, this);
        }
    }

    if (item) {
        addItem(item);
    }
}

QString Workspace::getName() const
{
    return _workspaceName;
}

void Workspace::setName(const QString &name)
{
    _workspaceName = name;
    if (_titleLabel) {
        _titleLabel->setText(name);
    }
}

void Workspace::addItem(AbstractWorkspaceItem *item)
{
    _items.append(item);

    if (_spacerItem) {
        _layout->removeItem(_spacerItem);
        delete _spacerItem;
        _spacerItem = nullptr;
    }

    if (auto resizableItem = static_cast<ResizableItem *>(item))
        connect(resizableItem, &ResizableItem::resized, this, &Workspace::adjustLayout);

    // item->setMinimumHeight(50);
    // item->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    item->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(item, &QWidget::customContextMenuRequested, this, [this, item](const QPoint &pos) {
        QMenu contextMenu(this);

        QAction *deleteAction = contextMenu.addAction("Удалить элемент");
        connect(deleteAction, &QAction::triggered, [item, this]() {
            removeItem(item);
        });

        contextMenu.exec(item->mapToGlobal(pos));
    });

    _layout->addWidget(item);

    adjustLayout();

    _spacerItem = new QSpacerItem(20, _contentWidget->height() - _contentWidget->minimumHeight(),
                                  QSizePolicy::Minimum, QSizePolicy::Fixed);
    _layout->addItem(_spacerItem);

    adjustLayout();
}

void Workspace::removeItem(AbstractWorkspaceItem *item)
{
    _items.removeOne(item);
    _layout->removeWidget(item);
    delete item;
    updateContentSize();
}

QJsonObject Workspace::serialize() const
{
    QJsonObject json;
    json["name"] = _workspaceName;

    QJsonArray itemArray;
    for (const AbstractWorkspaceItem *item : _items) {
        itemArray.append(item->serialize());
    }
    json["items"] = itemArray;

    return json;
}

void Workspace::deserialize(const QJsonObject &json)
{
    if (json.contains("name")) {
        setName(json["name"].toString());
    }

    if (json.contains("items")) {
        QJsonArray itemArray = json["items"].toArray();
        for (const QJsonValue &itemVal : itemArray) {
            QJsonObject itemObj = itemVal.toObject();
            QString type = itemObj["type"].toString();
            AbstractWorkspaceItem *item = nullptr;

            if (type == "TitleItem") {
                item = new TitleItem();
            } else if (type == "TextItem") {
                item = new TextItem();
            } else if (type == "ListItem") {
                item = new ListItem();
            } else if (type == "CheckboxItem") {
                item = new CheckboxItem();
            } else if (type == "ImageItem") {
                item = new ImageItem();
            } else if (type == "FileItem") {
                item = new FileItem();
            } else if (type == "NestedWorkspaceItem") {
                item = new NestedWorkspaceItem();
            }

            if (item) {
                item->deserialize(itemObj);
                addItem(item);
            }
        }
    }
}

void Workspace::adjustLayout()
{
    int currentY = 0;
    for (AbstractWorkspaceItem *item : _items) {
        QRect itemGeometry = item->geometry();
        itemGeometry.moveTop(currentY);
        item->setGeometry(itemGeometry);
        currentY += itemGeometry.height() + _layout->spacing();
        qDebug() << "qe" << item->height();
    }

    // if (_spacerItem) {
    //     _layout->removeItem(_spacerItem);
    //     delete _spacerItem;
    //     _spacerItem = nullptr;
    // }

    updateContentSize();

    // _spacerItem = new QSpacerItem(20, _contentWidget->height() - _contentWidget->minimumHeight(),
    //                               QSizePolicy::Minimum, QSizePolicy::Fixed);
}

void Workspace::updateContentSize()
{
    int totalHeight = 0;

    for (AbstractWorkspaceItem *item : _items) {
        totalHeight += item->height() + _layout->spacing();
    }

    if (_spacerItem) {
        totalHeight += _spacerItem->geometry().height() + _layout->spacing();
    }

    _contentWidget->setMinimumHeight(totalHeight);
    _contentWidget->updateGeometry();
}

QList<AbstractWorkspaceItem *> Workspace::getItems() const
{
    return _items;
}
