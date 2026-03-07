// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include <QtGui/qfont.h>
#include <private/qquickwindowmodule_p.h>

DS_BEGIN_NAMESPACE
class PopupWindow : public QQuickWindowQmlImpl
{
    Q_OBJECT
    Q_PROPERTY(QWindow *transientParent READ transientParent WRITE setTransientParent NOTIFY transientParentChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont RESET resetFont NOTIFY fontChanged FINAL)
    QML_NAMED_ELEMENT(PopupWindow)

public:
    PopupWindow(QWindow *parent = nullptr);

    QFont font() const;
    void setFont(const QFont &font);
    void resetFont();

Q_SIGNALS:
    void fontChanged();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void propagateFontToContentItem(const QFont &font);

    bool m_dragging;
    bool m_pressing;
    QFont m_font;
    bool m_hasExplicitFont = false;
};
DS_END_NAMESPACE
