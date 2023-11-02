// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"

#include <QQmlListProperty>

DS_BEGIN_NAMESPACE
namespace osd {

class KBLayout : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString key READ key NOTIFY keyChanged FINAL)
    Q_PROPERTY(QString text READ text NOTIFY textChanged FINAL)
public:
    KBLayout(const QString &key, const QString &text, QObject *parent = nullptr);

    QString key() const;
    QString text() const;

Q_SIGNALS:
    void keyChanged();
    void textChanged();

private:
    QString m_key;
    QString m_text;
};

class KBLayoutApplet : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(QString currentLayout READ currentLayout WRITE setCurrentLayout NOTIFY currentLayoutChanged FINAL)
    Q_PROPERTY(QQmlListProperty<KBLayout> layouts READ layouts NOTIFY layoutsChanged FINAL)

public:
    explicit KBLayoutApplet(QObject *parent = nullptr);
    virtual void init() override;

    QString currentLayout() const;
    QQmlListProperty<KBLayout> layouts();

    Q_INVOKABLE void sync();
    Q_INVOKABLE void next();

Q_SIGNALS:
    void currentLayoutChanged();
    void layoutsChanged();

private:
    void setCurrentLayout(const QString &newCurrentLayout);
    void fetchLayouts();
    void fetchCurrentLayout();

private:
    QList<KBLayout *> m_layouts;
    QString m_currentLayout;
};

}
DS_END_NAMESPACE
