// Copyright (C) 2022 ~ 2022 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DATETIMEDISPLAYER_H
#define DATETIMEDISPLAYER_H

#include "constants.h"

#include "org_deepin_dde_timedate1.h"

#include <QWidget>
#include <QFont>

#include <dtkcore_global.h>

namespace Dock { class TipsWidget; }

class DockPopupWindow;
class QMenu;
DCORE_BEGIN_NAMESPACE
class DConfig;
DCORE_END_NAMESPACE

using Timedate = org::deepin::dde::Timedate1;

class DateTimeDisplayer : public QWidget
{
    Q_OBJECT

private:
    struct DateTimeInfo {
        QString m_time;
        QString m_date;
        QRect m_timeRect;
        QRect m_dateRect;
    };

public:
    explicit DateTimeDisplayer(bool showMultiRow, QWidget *parent = nullptr);
    ~DateTimeDisplayer() override;
    void setPositon(Dock::Position position);
    void setOneRow(bool oneRow);
    QSize suitableSize() const;
    QSize suitableSize(const Dock::Position &position) const;

Q_SIGNALS:
    void requestUpdate();         // 当日期时间格式发生变化的时候，需要通知外面来更新窗口尺寸
    void requestDrawBackground(const QRect &rect);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool event(QEvent *event) override;

private:
    void updatePolicy();
    DateTimeInfo dateTimeInfo(const Dock::Position &position) const;
    void updateLastData(const DateTimeInfo &info);

    inline QString getTimeString() const;
    QString getTimeString(const Dock::Position &position) const;
    inline QString getDateString() const;
    QString getDateString(const Dock::Position &position) const;

    QPoint tipsPoint() const;
    void updateFont() const;

    void createMenuItem();
    QRect textRect(const QRect &sourceRect) const;
    void initDConfig();

private Q_SLOTS:
    void onTimeChanged();
    void onDateTimeFormatChanged();

private:
    Timedate *m_timedateInter;
    Dock::Position m_position;
    mutable QFont m_dateFont;
    mutable QFont m_timeFont;
    Dock::TipsWidget *m_tipsWidget;
    QMenu *m_menu;
    QSharedPointer<DockPopupWindow> m_tipPopupWindow;
    QTimer *m_tipsTimer;
    QString m_lastDateString;
    QString m_lastTimeString;
    int m_currentSize;
    bool m_oneRow;
    bool m_showMultiRow;
    int m_shortDateFormat;
    DTK_CORE_NAMESPACE::DConfig *m_config;
    QString m_shortDateFormatStr;
    QString m_shortTimeFormatStr;
    QString m_longDateFormatStr;
    QString m_longTimeFormatStr;
    QLocale m_locale;
};

#endif // DATETIMEDISPLAYER_H
