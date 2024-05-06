// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QUICKPROXYWIDGET_H
#define QUICKPROXYWIDGET_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QQuickItem>
#include <QQuickPaintedItem>

QT_BEGIN_NAMESPACE

class QuickProxyWidgetPrivate;

class QuickProxyWidget : public QQuickPaintedItem
{
    Q_OBJECT
public:
    QuickProxyWidget(QQuickItem* parent = nullptr);
    ~QuickProxyWidget();

    void setWidget(QWidget *widget);
    QWidget *widget() const;

    // void setGeometry(const QRectF &rect);
    virtual void paint(QPainter* painter) override;

protected:
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    bool event(QEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

#if QT_CONFIG(draganddrop)    
    virtual void dragEnterEvent(QDragEnterEvent *) override;
    virtual void dragMoveEvent(QDragMoveEvent *) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *) override;
    virtual void dropEvent(QDropEvent *) override;
#endif

    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

#if QT_CONFIG(wheelevent)
    virtual void wheelEvent(QWheelEvent *event) override;
#endif

private:
    Q_DISABLE_COPY(QuickProxyWidget)
    Q_DECLARE_PRIVATE(QuickProxyWidget);
};

QT_END_NAMESPACE

#endif
