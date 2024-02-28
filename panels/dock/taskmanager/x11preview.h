// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "appitem.h"
#include "dsglobal.h"
#include "abstractwindow.h"

#include <cstdint>

#include <QLabel>
#include <QWidget>
#include <DIconButton>
#include <QVBoxLayout>
#include <DBlurEffectWidget>

DS_BEGIN_NAMESPACE
DWIDGET_USE_NAMESPACE

namespace dock {
class X11WindowMonitor;
class X11WindowPreviewContent : public QWidget
{
    Q_OBJECT

public:
    X11WindowPreviewContent(const QPointer<AbstractWindow> &window, QWidget* parent = nullptr);
    void setPrewviewContentWindow(const QPointer<AbstractWindow> &window);

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

Q_SIGNALS:
    void entered(const QPointer<AbstractWindow> &window);
    void exited();
    void windowDestoried(X11WindowPreviewContent* widnow);

private:
    void fetchWindowPreview();

private:
    bool m_isHovered;
    bool m_isMinimized;

    QLabel* m_previewLabel;
    DIconButton* m_closeButton;
    QPixmap m_pixmap;
    QTimer *m_timer;

    QPointer<AbstractWindow> m_window;
};

class X11WindowPreviewContainer: public DBlurEffectWidget
{
    Q_OBJECT

public:
    explicit X11WindowPreviewContainer(X11WindowMonitor* monitor, QWidget *parent = nullptr);
    void showPreview(const QPointer<AppItem> &item, const QPointer<QWindow> &window, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction);
    void hidePreView();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    inline void clearSpecifiedCountPreviews(int count);
    inline void updateCurrentHoveredTitle();
    inline void initUI();

public Q_SLOTS:
    void updatePosition();

private Q_SLOTS:
    void addNeedPreviewWindow(const QPointer<AbstractWindow> &window);
    void updateHoveredWindow(AbstractWindow *window);
    void updateLayout();
    void callHide();

private:
    bool m_isPreviewEntered;
    int32_t m_isDockPreviewCount;

    X11WindowMonitor* m_x11Monitor;
    QVBoxLayout* m_mainLayout;
    QBoxLayout* m_contentLayout;
    QLabel* m_currentWindowIcon;
    QLabel* m_currentWindowTitle;
    DIconButton* m_closeAllButton;
    QTimer* m_hideTimer;
    QTimer* m_exitTimer;

    QRect m_hoverRect;

    int32_t m_previewXoffset;
    int32_t m_previewYoffset;
    uint32_t m_direction;

    QPointer<QWindow> m_baseWindow;
    QPointer<AppItem> m_previewItem;
    QPointer<AbstractWindow> m_hoveredWindow;

    QString m_currentWindowTitleStr;
};

}
DS_END_NAMESPACE
