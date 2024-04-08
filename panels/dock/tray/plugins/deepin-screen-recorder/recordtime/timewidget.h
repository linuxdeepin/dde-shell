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

class TimeWidget : public QWidget
{
    Q_OBJECT

    enum position {
        top = 0,
        right,
        bottom,
        left
    };

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
     * @brief sizeHint:返回控件大小
     * @return
     */
    QSize sizeHint() const override;

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
    QTimer *m_timer;
    QIcon *m_lightIcon;
    QIcon *m_shadeIcon;
    QIcon *m_currentIcon;
    QPixmap m_pixmap;
    QSize m_textSize;
    QTime m_baseTime;
    QString m_showTimeStr;
    bool m_bRefresh;
    int m_position;
    QBoxLayout *centralLayout;
    bool m_hover;
    bool m_pressed;
};

#endif // TIMEWIDGET_H
