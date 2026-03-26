// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>

DS_BEGIN_NAMESPACE
class PopupWindow : public QQuickApplicationWindow
{
    Q_OBJECT
    Q_PROPERTY(QWindow *transientParent READ transientParent WRITE setTransientParent NOTIFY transientParentChanged)
    Q_PROPERTY(bool x11GrabFocusTransition READ x11GrabFocusTransition NOTIFY x11GrabFocusTransitionChanged)
    QML_NAMED_ELEMENT(PopupWindow)

public:
    PopupWindow(QWindow *parent = nullptr);

    bool x11GrabFocusTransition() const { return m_x11GrabFocusTransition; }
    Q_INVOKABLE void setWindowGeometry(int px, int py, int pw, int ph);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
    void x11GrabFocusTransitionChanged();
    void x11FocusOutByGrab();
    void x11FocusInByUngrab();

private:
    void setX11GrabFocusTransition(bool transition);
    bool m_dragging;
    bool m_pressing;
    bool m_x11GrabFocusTransition = false;
};
DS_END_NAMESPACE
