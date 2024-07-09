// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QWindow>

namespace dock {

class DockPanel;
class DockPositioner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QRect bounding READ bounding WRITE setBounding NOTIFY boundingChanged FINAL)
    Q_PROPERTY(int x READ x NOTIFY xChanged FINAL)
    Q_PROPERTY(int y READ y NOTIFY yChanged FINAL)
    QML_NAMED_ELEMENT(DockPositioner)
    QML_ATTACHED(DockPositioner)
public:
    explicit DockPositioner(DockPanel *panel, QObject *parent = nullptr);
    virtual ~DockPositioner() override;

    static DockPositioner *qmlAttachedProperties(QObject *object);

    QRect bounding() const;
    void setBounding(const QRect &newBounding);
    int x() const;
    int y() const;
    QWindow *window() const;

    void setX(int x);
    void setY(int y);
public slots:
    void update();
    virtual void updatePosition();

signals:
    void boundingChanged();
    void xChanged();
    void yChanged();

protected:
    DockPanel *m_panel = nullptr;
    QRect m_bounding {};
    int m_x = 0;
    int m_y = 0;
    QTimer *m_positionTimer = nullptr;
};

class DockPanelPositioner : public DockPositioner
{
    Q_OBJECT
    Q_PROPERTY(int horizontalOffset READ horizontalOffset WRITE setHorizontalOffset RESET resetHorizontalOffset NOTIFY horizontalOffsetChanged FINAL)
    Q_PROPERTY(int vertialOffset READ vertialOffset WRITE setVertialOffset RESET resetVertialOffset NOTIFY vertialOffsetChanged FINAL)
    QML_NAMED_ELEMENT(DockPanelPositioner)
    QML_ATTACHED(DockPanelPositioner)
public:
    explicit DockPanelPositioner(DockPanel *panel, QObject *parent = nullptr);
    virtual ~DockPanelPositioner() override;

    static DockPanelPositioner *qmlAttachedProperties(QObject *object);

    int horizontalOffset() const;
    void setHorizontalOffset(int newHorizontalOffset);
    void resetHorizontalOffset();

    int vertialOffset() const;
    void setVertialOffset(int newVertialOffset);
    void resetVertialOffset();

public slots:
    virtual void updatePosition() override;

signals:
    void horizontalOffsetChanged();
    void vertialOffsetChanged();

private:
    int m_horizontalOffset = -1;
    int m_vertialOffset = -1;
};
}
QML_DECLARE_TYPE(dock::DockPanelPositioner)
QML_DECLARE_TYPEINFO(dock::DockPanelPositioner, QML_HAS_ATTACHED_PROPERTIES)
