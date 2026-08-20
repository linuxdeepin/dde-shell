// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"

#include <DConfig>

#include <QPointer>

DS_BEGIN_NAMESPACE
namespace keynotify
{

class TreelandKeyNotify;

class KeyNotifyApplet : public DApplet
{
    Q_OBJECT

public:
    explicit KeyNotifyApplet(QObject *parent = nullptr);
    ~KeyNotifyApplet() override;

    bool load() override;

private Q_SLOTS:
    void showCapsLockOsd(bool locked);
    void showNumLockOsd(bool locked);
    void updateCapsLockToggle(const QString &key);

private:
    void sendOsd(const QString &osdType);
    void initConfig();

private:
    QPointer<TreelandKeyNotify> m_keyNotify;
    Dtk::Core::DConfig *m_config = nullptr;
};

}
DS_END_NAMESPACE
