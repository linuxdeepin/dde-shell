// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "showdesktop.h"
#include "pluginfactory.h"

#include <QProcess>
#include <QGuiApplication>

DS_BEGIN_NAMESPACE
namespace dock {

ShowDesktop::ShowDesktop(QObject *parent)
    : DApplet(parent)
    , m_iconName("deepin-toggle-desktop")
{

}

bool ShowDesktop::load()
{
    return QStringLiteral("xcb") == QGuiApplication::platformName();
}

bool ShowDesktop::init()
{
    DApplet::init();
    return true;
}

void ShowDesktop::toggleShowDesktop()
{
    QProcess::startDetached("/usr/lib/deepin-daemon/desktop-toggle", QStringList());
}

QString ShowDesktop::iconName() const
{
    return m_iconName;
}

void ShowDesktop::setIconName(const QString& iconName)
{
    if (iconName != m_iconName) {
        m_iconName = iconName;
        Q_EMIT iconNameChanged();
    }
}

D_APPLET_CLASS(ShowDesktop)
}

DS_END_NAMESPACE

#include "showdesktop.moc"
