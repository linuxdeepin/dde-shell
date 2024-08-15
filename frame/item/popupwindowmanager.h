// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef POPUPWINDOWMANAGER_H
#define POPUPWINDOWMANAGER_H

#include "dockpopupwindow.h"

#include <QObject>
#include <QPointer>

class DockItem;

class PopupWindowManager : public QObject
{
    Q_OBJECT

public:
    enum PopupType {
        PopupType_Tips,
        PopupType_Applet
    };

public:
    PopupWindowManager(QWidget *parent = nullptr);
    ~PopupWindowManager();

    DockPopupWindow *appletPopup() const;
    DockPopupWindow *toAppletPopup(DockItem *item);
    DockPopupWindow *toTipsPopup(DockItem *item);
    DockPopupWindow *popup(DockItem* item) const;
    bool isModel(DockItem* item) const;
    bool existVisibleModelPopup() const;
    void hide(DockItem* item) const;
    void accept(DockItem* item) const;
    bool isVisible(DockItem* item) const;
    QWidget *getContent(DockItem* item) const;
    void show(DockItem* item, const QPoint &pos, const bool model = false);
    void disconnectAccept(DockItem *item) const;
    void connectAccept(DockItem *item) const;

public Q_SLOTS:
    void hidePopup(bool visible);

private:
    DockPopupWindow *createPopupWindow(bool noFocus, bool tellWmDock) const;
    inline DockPopupWindow *toPopup(DockPopupWindow *popup, DockItem *item);

private:
    DockPopupWindow *m_appletPopup;
    DockPopupWindow *m_tipsPopup;
    QMap<QPointer<DockItem>, DockPopupWindow *> m_bindMapping;
};

#endif
