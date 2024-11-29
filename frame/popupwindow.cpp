// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "popupwindow.h"

DS_BEGIN_NAMESPACE
PopupWindow::PopupWindow(QWindow *parent)
    : QQuickWindowQmlImpl(parent)
{
}

void PopupWindow::mouseReleaseEvent(QMouseEvent *event)
{
    QQuickWindowQmlImpl::mouseReleaseEvent(event);
    auto rect = geometry();
    if (!rect.contains(event->globalPosition().toPoint())) {
        close();
    }
}

DS_END_NAMESPACE
