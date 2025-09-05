// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

#include <DBlurEffectWidget>
#include <DGuiApplicationHelper>
#include <DIconButton>
#include <DLabel>
#include <DToolButton>
#include <QListView>
#include <QPointer>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>

DWIDGET_USE_NAMESPACE

namespace dock {
class X11WindowMonitor;
class DockItemWindowModel;
class AppItemWindowDeletegate;
class PreviewsListView;

class DIconButtonHoverFilter : public QObject
{
    Q_OBJECT public:
    explicit DIconButtonHoverFilter(QObject* parent = nullptr) : QObject(parent)
    {
    }

    void setButtonColor(QObject* watched, QColor color)
    {
        DIconButton* button = qobject_cast<DIconButton*>(watched);
        if (button) {
            QPalette pt = button->palette();
            pt.setColor(QPalette::Button, color);
            button->setPalette(pt);
        }
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::HoverEnter) {
            // 处理 HoverEnter 事件
            QColor color = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType ? Qt::white : Qt::black;
            color.setAlphaF(0.1);
            setButtonColor(watched, color);
        } else if (event->type() == QEvent::HoverLeave || event->type() == QEvent::Show) {
            // 处理 HoverLeave 和 show事件
            setButtonColor(watched, Qt::transparent);
        }
        return QObject::eventFilter(watched, event);
    }
};


class X11WindowPreviewContainer: public DBlurEffectWidget
{
    Q_OBJECT

public:
    explicit X11WindowPreviewContainer(X11WindowMonitor* monitor, QWidget *parent = nullptr);

    void
    showPreviewWithModel(QAbstractItemModel *sourceModel, const QPointer<QWindow> &window, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction);

    void hidePreView();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    inline void updatePreviewTitle(const QString& title);
    inline void initUI();
    inline void updateSize(int windowCount = -1);
    void updatePreviewIconFromBase64(const QString &base64Data);

public Q_SLOTS:
    void updatePosition();

private Q_SLOTS:
    void updateOrientation();
    void callHide();

private:
    bool m_isPreviewEntered;
    int32_t m_isDockPreviewCount;

    QPointer<X11WindowMonitor> m_monitor;
    QAbstractItemModel *m_sourceModel;
    PreviewsListView* m_view;
    QWidget *m_titleWidget;

    QLabel* m_previewIcon;
    DLabel* m_previewTitle;
    DIconButton* m_closeAllButton;

    QTimer* m_hideTimer;

    int32_t m_previewXoffset;
    int32_t m_previewYoffset;
    uint32_t m_direction;

    QPointer<QWindow> m_baseWindow;

    QString m_previewTitleStr;
};

}
