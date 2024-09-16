// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef XCB_GET_H
#define XCB_GET_H
#include "qobjectdefs.h"
#include <xcb/xcb.h>
#include <unistd.h>
#include <err.h>
#include <QObject>

enum { INACTIVE, ACTIVE };

/* global variables */
static xcb_connection_t		*conn;
static xcb_screen_t		*scr;
static xcb_window_t		 focuswin;


typedef struct {
    xcb_window_t window;
    char *name;
    time_t create_time;
    // 可以添加其他窗口信息的字段
} WindowInfo;
Q_DECLARE_METATYPE(WindowInfo)
class xcb_get : public QObject
{
    Q_OBJECT
signals:
    void windowCreated(xcb_window_t window, const char *name);
    void windowDestroyed(xcb_window_t window);
    void windowCreated_qiantai(xcb_window_t window, const char *name);
    void windowCreated_houtai(xcb_window_t window, const char *name);

public slots:
    void handle_create_notify_event(xcb_create_notify_event_t *e);
public:
   void save_destroy_window(xcb_window_t window, char *name);
    xcb_get();
    void print_window_info(xcb_window_t window_id);
    static void cleanup(void);
    static  int deploy(void);
    void handle_destroy_notify_event(xcb_destroy_notify_event_t *e);
    void events_loop(void);
    void get_children(xcb_connection_t* , xcb_window_t , xcb_window_t** , int* , xcb_get_geometry_reply_t*** );
    void get_name(xcb_connection_t* , xcb_window_t , char** , int* );
    void get_all_windows();
};

#endif // XCB_GET_H
