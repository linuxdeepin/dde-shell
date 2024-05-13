// SPDX-FileCopyrightText: 2019 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "settingbutton.h"

#include <QHBoxLayout>

#include <DFontSizeManager>
#include <DPaletteHelper>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

SettingButton::SettingButton(const QString &txt, QWidget *parent)
    : QFrame(parent)
    , m_hover(false)
    , m_descriptionLabel(new DLabel(txt, this))
{
    initUI();
}

SettingButton::~SettingButton()
{

}

void SettingButton::initUI()
{
    setFixedHeight(36);
    setForegroundRole(QPalette::BrightText);

    m_descriptionLabel->setElideMode(Qt::ElideRight);
    m_descriptionLabel->setForegroundRole(foregroundRole());
    DFontSizeManager::instance()->bind(m_descriptionLabel, DFontSizeManager::T6);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 0, 10, 0);
    layout->addWidget(m_descriptionLabel);
    layout->addStretch();
}

bool SettingButton::event(QEvent* e)
{
    switch (e->type()) {
    case QEvent::Leave:
    case QEvent::Enter:
        m_hover = e->type() == QEvent::Enter;
        update();
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

void SettingButton::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e)
    QPainter p(this);
    QPalette palette = this->palette();
    QColor bgColor, textColor;
    if (m_hover) {
        textColor = palette.highlightedText().color();
        bgColor = palette.color(QPalette::Active, QPalette::Highlight);
    } else {
        textColor = palette.brightText().color();
        bgColor = palette.brightText().color();
        bgColor.setAlphaF(0.05);
    }
    palette.setBrush(QPalette::BrightText, textColor);
    m_descriptionLabel->setPalette(palette);

    p.setBrush(bgColor);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect(), 8, 8);
    return QFrame::paintEvent(e);
}

void SettingButton::mouseReleaseEvent(QMouseEvent* event)
{
    if (underMouse()) {
        Q_EMIT clicked();
    }

    return QWidget::mouseReleaseEvent(event);
}