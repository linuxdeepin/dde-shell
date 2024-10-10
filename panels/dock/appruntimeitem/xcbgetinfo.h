// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "qobjectdefs.h"
#include <xcb/xcb.h>
#include <unistd.h>
#include <err.h>
#include <QObject>

#include "x11utils.h"


typedef struct {
    xcb_window_t window;
    char *name;
    time_t createTime;
} WindowInfo;
Q_DECLARE_METATYPE(WindowInfo)

enum { INACTIVE, ACTIVE };

/* global variables */
static xcb_connection_t		*conn;
static xcb_screen_t		*scr;
static xcb_window_t		 focuswin;

namespace dock {

class XcbGetInfo : public QObject
{
    Q_OBJECT
signals:
    void windowCreated(xcb_window_t window, const char *name);
    void windowDestroyed(xcb_window_t window);
    void windowCreatedForeground(xcb_window_t window, const char *name);
    void windowCreatedBackground(xcb_window_t window, const char *name);

public slots:
    void handleCreateNotifyEvent(xcb_create_notify_event_t *e);
public:
   void saveDestroyWindow(xcb_window_t window, char *name);
    XcbGetInfo();
    void printWindowInfo(xcb_window_t window_id);
    static void cleanup(void);
    static  int deploy(void);
    void handleDestroyNotifyEvent(xcb_destroy_notify_event_t *e);
    void eventsLoop(void);
    void getChildren(xcb_connection_t* , xcb_window_t , xcb_window_t** , int* , xcb_get_geometry_reply_t*** );
    void getName(xcb_connection_t* , xcb_window_t , char** , int* );
    void getAllWindows();
};

}
