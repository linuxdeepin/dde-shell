// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xcbthread.h"

#include <QDebug>

namespace dock {
XcbThread::XcbThread(QObject *parent) : QThread(parent)
{

}

XcbThread::~XcbThread()
{

}

void XcbThread::run()
{
    if (xcbGetInfo.deploy() < 0) {
        qWarning() << "Error connecting to X";
        return;
    }
    connect(&xcbGetInfo, &XcbGetInfo::windowCreatedBackground, this, [this](xcb_window_t window, const char *name) {
        Q_EMIT windowInfoChangedBackground(QString(name), window);
    });
    connect(&xcbGetInfo, &XcbGetInfo::windowCreatedForeground, this, [this](xcb_window_t window, const char *name) {
        Q_EMIT windowInfoChangedForeground(QString(name), window);
    });
    xcbGetInfo.getAllWindows();
    connect(&xcbGetInfo, &XcbGetInfo::windowCreated, this, [this](xcb_window_t window, const char *name) {
        Q_EMIT windowInfoChanged(QString(name), window);
    });
    connect(&xcbGetInfo, &XcbGetInfo::windowDestroyed, this, [this](xcb_window_t window) {
        Q_EMIT windowDestroyChanged(window);
    });
  //  for (;;) {
        xcbGetInfo.eventsLoop();
   // }
}
}
