// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xcbgetinfo.h"
#include <QDebug>

#define CLEANMASK(m)	((m & ~0x80))
#define X11 X11Utils::instance()
namespace dock {
XcbGetInfo::XcbGetInfo()
{

}

void XcbGetInfo::printWindowInfo(xcb_window_t window_id)
{
    xcb_get_geometry_cookie_t geom_cookie;
    xcb_get_geometry_reply_t *geom;

    // Get the window's geometry information
    geom_cookie = xcb_get_geometry(conn, window_id);
    geom = xcb_get_geometry_reply(conn, geom_cookie, NULL);
    // Get the window's name information
    xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, window_id, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 0, 100);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);

    char* name =NULL;
    if (reply) {
        if (xcb_get_property_value_length(reply) > 0) {
            name = strndup((char *)xcb_get_property_value(reply), xcb_get_property_value_length(reply));
        }
        free(reply);
    }

    if (geom) {
        if(name!=NULL){
            qDebug()<<"Window ID:"<<window_id;
            qDebug()<<"Position:"<<geom->x<<geom->y;
            qDebug()<<"Name ID: "<<name;
        }
        free(geom);
    }
}

void XcbGetInfo::cleanup()
{
    /* graceful exit */
    if (conn != NULL)
        xcb_disconnect(conn);
}

int XcbGetInfo::deploy()
{
    /* init xcb and grab events */
    uint32_t values[2];
    int mask;

    if (xcb_connection_has_error(conn = xcb_connect(NULL, NULL)))
        return -1;
    // conn = X11->getXcbConnection();
    focuswin = X11->getRootWindow();

    mask = XCB_CW_EVENT_MASK;
    values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    xcb_change_window_attributes_checked(conn, X11->getRootWindow(), mask, values);
    xcb_flush(conn);
    return 0;
}

void XcbGetInfo::handleCreateNotifyEvent(xcb_create_notify_event_t *e)
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
            printWindowInfo(e->window);
            //Q_EMIT signal with window ID and name
            Q_EMIT windowCreated(e->window, name);
        }
        free(reply);
    }
}

void XcbGetInfo::handleDestroyNotifyEvent(xcb_destroy_notify_event_t *e)
{
    Q_EMIT windowDestroyed(e->window);
}
void XcbGetInfo::eventsLoop()
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
            handleCreateNotifyEvent(e);
        }
            break;
        case XCB_DESTROY_NOTIFY: {
            xcb_destroy_notify_event_t *e;
            e = (xcb_destroy_notify_event_t *)ev;
            handleDestroyNotifyEvent(e);
        } break;

        }

        xcb_flush(conn);
        free(ev);
    }
}



void XcbGetInfo::getChildren(xcb_connection_t* c, xcb_window_t window, xcb_window_t** children, int* count, xcb_get_geometry_reply_t*** geoms)
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

void XcbGetInfo::getName(xcb_connection_t* c, xcb_window_t window, char** name, int* length)
{
    *length = 0;
    *name = NULL;

    xcb_get_property_cookie_t cookie = xcb_get_property(c, 0, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 0, 100);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(c, cookie, NULL);
    if (xcb_get_property_value_length(reply) > 0) {
        *length = xcb_get_property_value_length(reply);
        *name = (char*)malloc(*length + 1);
        if (*name) {
            memcpy(*name, xcb_get_property_value(reply), *length);
            (*name)[*length] = '\0';
        }
        free(reply);
    }
}

void XcbGetInfo::getAllWindows(){
    // Get children windows and their geometries
    const xcb_setup_t *setup = xcb_get_setup(conn);
    xcb_screen_t *screen = xcb_setup_roots_iterator(setup).data;
    xcb_window_t rootWindow = screen->root;
    int nChildren;
    xcb_window_t *children;
    xcb_get_geometry_reply_t **geoms;
    getChildren(conn, rootWindow, &children, &nChildren, &geoms);

    // Print information for each child window
    for (int i = 0; i < nChildren; i++) {
        xcb_window_t wid = children[i];
        xcb_get_geometry_reply_t *geom = geoms[i];

        int length;
        char *name;
        getName(conn, wid, &name, &length);

        // Print window ID, name, x, y coordinates
        if(name!=NULL){
            if(geom->width>10||geom->height>10){
                Q_EMIT windowCreatedForeground(wid,name);
            }
            else{
                Q_EMIT windowCreatedBackground(wid,name);
            }
            free(geom);
        }
    }
    // Cleanup
    free(geoms);
}
}
