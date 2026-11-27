// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "dlayershellwindow.h"

#include <QObject>
#include <QPointer>
#include <QScreen>
#include <QWindow>
#include <QTimer>

#include <xcb/xcb.h>
#include <xcb/xproto.h>

class xcb_connection_t;
DS_BEGIN_NAMESPACE
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
    void onScopeChanged();
    void onInputRegionChanged();
    // void onKeyboardInteractivityChanged();

private:
    // 该 surface 所属的 output。创建时锁存，之后不再随窗口当前所在屏漂移。
    QScreen *targetScreen() const;

    QWindow* m_window;
    DLayerShellWindow* m_dlayerShellWindow;
    QPointer<QScreen> m_screen;
    QTimer m_exclusionZoneChangedTimer;
};
DS_END_NAMESPACE
