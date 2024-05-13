// SPDX-FileCopyrightText: 2019 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef SETTING_BUTTON_H
#define SETTING_BUTTON_H

#include <QFrame>

#include <DLabel>

class SettingButton : public QFrame
{
    Q_OBJECT
public:
    explicit SettingButton(const QString &txt, QWidget *parent = nullptr);
    ~SettingButton();

signals:
    void clicked();

protected:
    bool event(QEvent* e) override;
    void paintEvent(QPaintEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void initUI();

private:
    bool m_hover;
    Dtk::Widget::DLabel *m_descriptionLabel;
};

#endif
