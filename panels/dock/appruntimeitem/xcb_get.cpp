// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xcb_get.h"
#include <QDebug>

#define CLEANMASK(m)	((m & ~0x80))

xcb_get::xcb_get()
{

}

void xcb_get::print_window_info(xcb_window_t window_id)
{
    xcb_get_geometry_cookie_t geom_cookie;
    xcb_get_geometry_reply_t *geom;

    // 获取窗口的几何信息
    geom_cookie = xcb_get_geometry(conn, window_id);
    geom = xcb_get_geometry_reply(conn, geom_cookie, NULL);
    //获取窗口的名称信息
    xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, window_id, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 0, 100);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);

    char* name =NULL;
    if (reply) {
        if (xcb_get_property_value_length(reply) > 0) {
            // 分配足够的内存并复制名称
            name = strndup((char *)xcb_get_property_value(reply), xcb_get_property_value_length(reply));
        }
        free(reply);
    }

    if (geom) {
        if(name!=NULL){
            qDebug()<<"Window ID:"<<window_id;
            qDebug()<<"Position:"<<geom->x<<geom->y;
            qDebug()<<"Name ID: "<<name;
            // 输出其他信息
        }
        free(geom);
    }
}

void xcb_get::cleanup()
{
    /* graceful exit */
    if (conn != NULL)
        xcb_disconnect(conn);
}

int xcb_get::deploy()
{
    atexit(cleanup);
    /* init xcb and grab events */
    uint32_t values[2];
    int mask;

    if (xcb_connection_has_error(conn = xcb_connect(NULL, NULL))) //进行XCB连接
        return -1;

    scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    focuswin = scr->root;//获取root的id

    mask = XCB_CW_EVENT_MASK;
    values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    xcb_change_window_attributes_checked(conn, scr->root, mask, values);
    xcb_flush(conn);
    return 0;
}

void xcb_get::handle_create_notify_event(xcb_create_notify_event_t *e)
{
    if (!e->override_redirect) {
        char *name = NULL;
        xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, e->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 0, 100);
        xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);
        if (reply) {
            if (xcb_get_property_value_length(reply) > 0) {
                name = (char *)xcb_get_property_value(reply);
            }
        }

        if (name != NULL) {
            qDebug()<<"Window created: ID=,"<<e->window<<"Name="<<name;
            print_window_info(e->window);
            // Emit signal with window ID and name
            emit windowCreated(e->window, name);
        }
        free(reply);
    }
}

void xcb_get::handle_destroy_notify_event(xcb_destroy_notify_event_t *e)
{
    emit windowDestroyed(e->window);
}
void xcb_get::events_loop()
{
    xcb_generic_event_t *ev;
    /* loop */
    for (;;) {
        ev = xcb_wait_for_event(conn);

        if (!ev)
            errx(1, "xcb connection broken");

        switch (CLEANMASK(ev->response_type)) {

        case XCB_CREATE_NOTIFY: {
            xcb_create_notify_event_t *e;
            e = (xcb_create_notify_event_t *)ev;
            handle_create_notify_event(e);
        }
            break;
        case XCB_DESTROY_NOTIFY: {
            xcb_destroy_notify_event_t *e;
            e = (xcb_destroy_notify_event_t *)ev;
            handle_destroy_notify_event(e);
        } break;

        }

        xcb_flush(conn);
        free(ev);
    }
}



void xcb_get::get_children(xcb_connection_t* c, xcb_window_t window, xcb_window_t** children, int* count, xcb_get_geometry_reply_t*** geoms)
{
    *count = 0;
    *children = NULL;

    // Query tree to get children windows
    xcb_query_tree_cookie_t cookie = xcb_query_tree(c, window);
    xcb_query_tree_reply_t *reply = xcb_query_tree_reply(c, cookie, NULL);
    if (!reply) {
        return;
    }

    *count = xcb_query_tree_children_length(reply);
    *children = xcb_query_tree_children(reply);

    // Allocate memory for geometry replies
    *geoms = (xcb_get_geometry_reply_t **)malloc(*count * sizeof(xcb_get_geometry_reply_t *));
    if (*geoms == NULL) {
        free(reply);
        return;
    }

    // Get geometry for each child window
    for (int i = 0; i < *count; i++) {
        (*geoms)[i] = xcb_get_geometry_reply(c, xcb_get_geometry(c, (*children)[i]), NULL);
    }

    free(reply);
}

void xcb_get::get_name(xcb_connection_t* c, xcb_window_t window, char** name, int* length)
{
    *length = 0;
    *name = NULL;

    xcb_get_property_cookie_t cookie = xcb_get_property(c, 0, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 0, 100);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(c, cookie, NULL);
    if (xcb_get_property_value_length(reply) > 0) {
        *length = xcb_get_property_value_length(reply);
        // 分配新的内存并复制数据
        *name = (char*)malloc(*length + 1); // +1 for null terminator
        if (*name) {
            memcpy(*name, xcb_get_property_value(reply), *length);
            (*name)[*length] = '\0'; // 确保字符串以 '\0' 结束
        }
        free(reply); // 释放 reply
    }
}

void xcb_get::get_all_windows(){
    // Get children windows and their geometries
    const xcb_setup_t *setup = xcb_get_setup(conn);
    xcb_screen_t *screen = xcb_setup_roots_iterator(setup).data;
    xcb_window_t rootWindow = screen->root;
    int nChildren;
    xcb_window_t *children;
    xcb_get_geometry_reply_t **geoms;
    get_children(conn, rootWindow, &children, &nChildren, &geoms);

    // Print information for each child window
    for (int i = 0; i < nChildren; i++) {
        xcb_window_t wid = children[i];
        xcb_get_geometry_reply_t *geom = geoms[i];

        int length;
        char *name;
        get_name(conn, wid, &name, &length);

        // Print window ID, name, x, y coordinates
        if(name!=NULL){
            if(geom->width>10||geom->height>10){
                emit windowCreated_qiantai(wid,name);
                qDebug() << "qiantaiName:" << name<< "Area: (" << geom->width << ", " << geom->height << ")";
                printf("qiantai:Window ID: %u, Name: %s,Coordinates: (%d, %d),area:(%d,%d)\n", wid, name,geom->x, geom->y,geom->width,geom->height);
            }
            else{
                emit windowCreated_houtai(wid,name);
                qDebug() << "houtaiName:" << name<< "Area: (" << geom->width << ", " << geom->height << ")";
                printf("houtai:Window ID: %u, Name: %s,Coordinates: (%d, %d),area:(%d,%d)\n", wid, name,geom->x, geom->y,geom->width,geom->height);
            }
            free(geom);
        }
    }
    // Cleanup
    free(geoms);
}

