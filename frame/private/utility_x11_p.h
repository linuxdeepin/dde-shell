// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "dsutility.h"

#include <QObject>
#include <QWindow>
#include <QGuiApplication>

DS_BEGIN_NAMESPACE

class X11Utility : public Utility
{
public:
    explicit X11Utility();
    _XDisplay *getDisplay();

    void deliverMouseEvent(uint8_t qMouseButton, int x, int y);

    bool grabKeyboard(QWindow *target, bool grab = true) override;
    bool grabMouse(QWindow *target, bool grab = true) override;
private:
    _XDisplay *m_display = nullptr;
};

class MouseGrabEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit MouseGrabEventFilter(QWindow *target);

    bool eventFilter(QObject *watched, QEvent *event) override;
signals:
    void outsideMousePressed();
private:
    void mousePressEvent(QMouseEvent *e);

    QWindow *m_target = nullptr;
};

DS_END_NAMESPACE
