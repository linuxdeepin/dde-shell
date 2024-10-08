// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>

#include "abstractdesktopinfo.h"

namespace apps {
class AppItem : public QObject
{
    Q_OBJECT

    // READ Only from desktopfile
    Q_PROPERTY(QString desktopId READ desktopId FINAL CONSTANT)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY iconNameChanged)
    Q_PROPERTY(QString startupWMClass READ startupWMClass NOTIFY startupWMClassChanged)
    Q_PROPERTY(bool nodisplay READ nodisplay NOTIFY nodisplayChanged)
    Q_PROPERTY(DDECategories ddeCategories READ ddeCategories NOTIFY ddeCategoriesChanged)
    Q_PROPERTY(QString actions READ actions NOTIFY actionsChanged)
    Q_PROPERTY(qint64 lastLaunchedTime READ lastLaunchedTime NOTIFY lastLaunchedTimeChanged)
    Q_PROPERTY(qint64 installedTime READ installedTime NOTIFY installedTimeChanged)
    Q_PROPERTY(qint64 launchedTimes READ launchedTimes NOTIFY launchedTimesChanged)

    Q_PROPERTY(int groupId READ groupId NOTIFY groupIdChanged)
    Q_PROPERTY(bool docked READ docked NOTIFY dockedChanged)

    // READ and WRITE Status
    Q_PROPERTY(bool onDesktop READ onDesktop WRITE setOnDesktop NOTIFY onDesktopChanged)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)

public:
     // This is different from the menu-spec Main Categories list.
    enum DDECategories {
        Internet,               // 网络模式
        Chat,                   // 社交模式
        Music,                  // 音乐模式
        Video,                  // 视频模式
        Graphics,               // 图形图像
        Game,                   //
        Office,                 // 办公模式
        Reading,                // 阅读模式
        Development,            // 编程开发模式
        System,                 // 系统管理模式
        Others,
    };
    Q_ENUM(DDECategories)

public:
    explicit AppItem(AbstractDesktopInfo * desktopInfo, QObject* parent = nullptr);
    ~AppItem();

    void launch(const QString& action = {}, const QStringList &fields = {}, const QVariantMap &options = {});

    // desktop static info
    QString desktopId() const;
    QString name() const;
    QString iconName() const;
    bool nodisplay() const;
    QString startupWMClass() const;
    DDECategories ddeCategories() const;
    QString actions() const;
    quint64 lastLaunchedTime() const;
    quint64 installedTime() const;
    quint64 launchedTimes() const;
    QString deepinVendor() const;
    bool autoStart() const;
    void setAutoStart(bool autoStart);

    // shell runing data
    int groupId() const;
    void setGroupId(const uint &groudId, const int &pos);

    bool onDesktop() const;
    void setOnDesktop(bool onDesktop);

    bool docked() const;
    void setDocked(bool docked);

Q_SIGNALS:
    void nameChanged(void);
    void iconNameChanged(void);
    void nodisplayChanged(void);
    void startupWMClassChanged(void);
    void ddeCategoriesChanged(void);
    void actionsChanged(void);
    void lastLaunchedTimeChanged(void);
    void installedTimeChanged(void);
    void autoStartChanged(void);
    void onDesktopChanged(void);

    void launchedTimesChanged(void);
    void groupIdChanged(void);
    void dockedChanged(void);

private:
    AbstractDesktopInfo *m_desktopInfo;
};
}
