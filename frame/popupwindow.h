// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include <private/qquickwindowmodule_p.h>

DS_BEGIN_NAMESPACE
class PopupWindow : public QQuickWindowQmlImpl
{
    Q_OBJECT
    Q_PROPERTY(QWindow *transientParent READ transientParent WRITE setTransientParent NOTIFY transientParentChanged)
    QML_NAMED_ELEMENT(PopupWindow)

public:
    PopupWindow(QWindow *parent = nullptr);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
};
DS_END_NAMESPACE
