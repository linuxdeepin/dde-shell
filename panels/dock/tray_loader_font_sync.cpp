// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QEvent>
#include <QFont>
#include <QFontDatabase>
#include <QMetaObject>
#include <QPointer>
#include <QTimer>
#include <QWidget>

namespace {

QString loadDataFontFamily()
{
    static QString dataFontFamily;
    static bool initialized = false;

    if (initialized) {
        return dataFontFamily;
    }

    initialized = true;
    const int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/tray_loader_font_sync/fonts/ElmsSans-Regular.ttf"));
    if (fontId < 0) {
        return dataFontFamily;
    }

    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (!families.isEmpty()) {
        dataFontFamily = families.constFirst();
    }

    return dataFontFamily;
}

bool isDatetimeRootWidget(const QWidget *widget)
{
    if (!widget) {
        return false;
    }

    const QByteArray className = widget->metaObject()->className();
    return className == "DatetimeWidget" || className == "SidebarCalendarWidget";
}

QWidget *datetimeRootWidget(QWidget *widget)
{
    QWidget *current = widget;
    while (current) {
        if (isDatetimeRootWidget(current)) {
            return current;
        }

        current = current->parentWidget();
    }

    return nullptr;
}

void applyFontFamilyRecursively(QWidget *widget, const QString &family)
{
    if (!widget || family.isEmpty()) {
        return;
    }

    QFont font = widget->font();
    if (font.family() != family) {
        font.setFamily(family);
        widget->setFont(font);
    }

    const auto children = widget->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children) {
        applyFontFamilyRecursively(child, family);
    }
}

class TrayLoaderFontSync final : public QObject
{
public:
    explicit TrayLoaderFontSync(QObject *parent = nullptr)
        : QObject(parent)
    {
        if (!qApp) {
            return;
        }

        qApp->installEventFilter(this);
        QTimer::singleShot(0, this, [this] {
            refreshDatetimeWidgets();
        });
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        auto *widget = qobject_cast<QWidget *>(watched);
        if (!widget) {
            return QObject::eventFilter(watched, event);
        }

        switch (event->type()) {
        case QEvent::Show:
        case QEvent::Polish:
        case QEvent::ApplicationFontChange:
        case QEvent::FontChange:
            scheduleRefresh(widget);
            break;
        default:
            break;
        }

        return QObject::eventFilter(watched, event);
    }

private:
    void refreshDatetimeWidgets()
    {
        const QString family = loadDataFontFamily();
        if (family.isEmpty()) {
            return;
        }

        const auto widgets = QApplication::allWidgets();
        for (QWidget *widget : widgets) {
            if (isDatetimeRootWidget(widget)) {
                applyFontFamilyRecursively(widget, family);
            }
        }
    }

    void scheduleRefresh(QWidget *widget)
    {
        QWidget *root = datetimeRootWidget(widget);
        if (!root) {
            return;
        }

        QPointer<QWidget> rootGuard(root);
        QTimer::singleShot(0, this, [rootGuard] {
            if (!rootGuard) {
                return;
            }

            const QString family = loadDataFontFamily();
            if (family.isEmpty()) {
                return;
            }

            applyFontFamilyRecursively(rootGuard, family);
        });
    }
};

void initTrayLoaderFontSync()
{
    if (!qApp) {
        return;
    }

    static auto *fontSync = new TrayLoaderFontSync(qApp);
    Q_UNUSED(fontSync)
}

} // namespace

Q_COREAPP_STARTUP_FUNCTION(initTrayLoaderFontSync)
