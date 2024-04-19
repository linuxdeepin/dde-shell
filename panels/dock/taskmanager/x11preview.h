// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "appitem.h"
#include "dsglobal.h"

#include <cstdint>

#include <QLabel>
#include <QWidget>
#include <QListView>
#include <DIconButton>
#include <QVBoxLayout>
#include <DBlurEffectWidget>

DWIDGET_USE_NAMESPACE

namespace dock {
class X11WindowMonitor;
class AppItemWindowModel;
class AppItemWindowDeletegate;
class PreviewsListView;

class X11WindowPreviewContainer: public DBlurEffectWidget
{
    Q_OBJECT

public:
    explicit X11WindowPreviewContainer(X11WindowMonitor* monitor, QWidget *parent = nullptr);
    void showPreview(const QPointer<AppItem> &item, const QPointer<QWindow> &window, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction);
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
    inline void updateSize();

public Q_SLOTS:
    void updatePosition();

private Q_SLOTS:
    void updateOrientation();
    void callHide();

private:
    bool m_isPreviewEntered;
    int32_t m_isDockPreviewCount;

    X11WindowMonitor* m_monitor;

    AppItemWindowModel* m_model;
    PreviewsListView* m_view;

    QLabel* m_previewIcon;
    QLabel* m_previewTitle;
    DIconButton* m_closeAllButton;

    QTimer* m_hideTimer;

    int32_t m_previewXoffset;
    int32_t m_previewYoffset;
    uint32_t m_direction;

    QPointer<QWindow> m_baseWindow;
    QPointer<AppItem> m_previewItem;

    QString m_previewTitleStr;
};

}
