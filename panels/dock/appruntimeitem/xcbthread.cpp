// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xcbthread.h"

#include <QDebug>

XcbThread::XcbThread(QObject *parent) : QThread(parent)
{

}

XcbThread::~XcbThread()
{

}

void XcbThread::run()
{
    if (xcb_test.deploy() < 0) {
        qWarning() << "Error connecting to X";
        return;
    }
    connect(&xcb_test, &xcb_get::windowCreated_houtai, this, [this](xcb_window_t window, const char *name) {
        emit windowInfoChanged_houtai(QString(name), window);
    });
    connect(&xcb_test, &xcb_get::windowCreated_qiantai, this, [this](xcb_window_t window, const char *name) {
        emit windowInfoChanged_qiantai(QString(name), window);
    });
    xcb_test.get_all_windows();
    connect(&xcb_test, &xcb_get::windowCreated, this, [this](xcb_window_t window, const char *name) {
        emit windowInfoChanged(QString(name), window);
    });
    connect(&xcb_test, &xcb_get::windowDestroyed, this, [this](xcb_window_t window) {
        emit windowDestroyChanged(window);
    });
    for (;;) {
        xcb_test.events_loop();
    }
}

