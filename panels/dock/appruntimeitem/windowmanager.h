// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QObject>
#include <QAbstractListModel>
#include <QList>
#include <QDateTime>

struct WindowInfo_1 {
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(uint id MEMBER id)
    Q_PROPERTY(QDateTime startTime MEMBER startTime)
public:
    QString name;
    uint id;
    QDateTime startTime;
    bool operator==(const WindowInfo_1 &other) const {
           return name == other.name && id == other.id;
       }
};


class WindowManager : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        IdRole,
        StartTimeRole // 新增角色
    };
    Q_ENUM(Roles)

    WindowManager(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVector<WindowInfo_1> windowList() const;

    void setWindowInfo_qiantai(const QString &name, uint id);
    void setWindowInfo_houtai(const QString &name, uint id);
    void WindowDetroyInfo(uint id);

private:
    QList<WindowInfo_1> m_windowList;
};

#endif // WINDOWMANAGER_H
