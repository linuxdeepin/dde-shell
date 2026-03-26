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
    QML_NAMED_ELEMENT(PopupWindow)

public:
    PopupWindow(QWindow *parent = nullptr);
    Q_INVOKABLE void setWindowGeometry(int px, int py, int pw, int ph);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    bool m_dragging;
    bool m_pressing;
};
DS_END_NAMESPACE
