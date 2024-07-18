// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TIMEWIDGET_H
#define TIMEWIDGET_H

#include <QWidget>
#include <QSettings>
#include <QTime>
#include <QIcon>
#include <QBoxLayout>
#include <QLabel>

#include <DGuiApplicationHelper>

#include "constants.h"

class TimeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimeWidget(QWidget *parent = nullptr);
    ~TimeWidget();
    bool enabled();

    /**
     * @brief start:开始计时
     */
    void start();

    /**
     * @brief stop:停止计时
     */
    void stop();

    /**
     * @brief 是否是wayland协议
     * @return
     */
    bool isWaylandProtocol();

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void leaveEvent(QEvent *e) override;
    /**
     * @brief 创建缓存文件，只有wayland模式下的mips或部分arm架构适用
     */
    void createCacheFile();
public slots:
    /**
     * @brief onPositionChanged:dde-dock位置变化通知
     * @param value
     */
    void onPositionChanged(int value);
private slots:
    /**
     * @brief onTimeout:更新数据
     */
    void onTimeout();

private:
    void updateIcon();

private:
    QTimer *m_timer;
    QIcon *m_lightIcon;
    QIcon *m_shadeIcon;
    QIcon *m_currentIcon;
    QLabel *m_iconLabel;
    QLabel *m_textLabel;
    QPixmap m_pixmap;
    QTime m_baseTime;
    bool m_bRefresh;
    int m_position;
    bool m_hover;
    bool m_pressed;
};

#endif // TIMEWIDGET_H
