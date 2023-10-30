// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dlayershellwindow.h"
#include <QObject>
#include <QWindow>

#include <qobject.h>
#include <qtmetamacros.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

class xcb_connection_t;

class LayerShellEmulation : public QObject
{
    Q_OBJECT
public:
    explicit LayerShellEmulation(QWindow* window, QObject* parent = nullptr);

private slots:
    void onLayerChanged();
    // margins or anchor changed
    void onPositionChanged();
    void onExclusionZoneChanged();
    // void onKeyboardInteractivityChanged();

private:
    QWindow* m_window;
    DS_NAMESPACE::DLayerShellWindow* m_dlayerShellWindow;
};
