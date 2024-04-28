// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "uosaiwidget.h"

#include <DGuiApplicationHelper>
#include <DStyle>
#include <DFontSizeManager>

#include <QSvgRenderer>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QIcon>
#include <QPainterPath>
#include <QLabel>
#include <QVBoxLayout>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace uos_ai;

#define PANEL_ICON_SIZE  20

UosAiWidget::UosAiWidget(QWidget *parent)
    : QWidget(parent)
    , m_hover(false)
    , m_pressed(false)
{
    setMouseTracking(true);
    setMinimumSize(PLUGIN_BACKGROUND_MIN_SIZE, PLUGIN_BACKGROUND_MIN_SIZE);
    loadSvg();
}

void UosAiWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);

    QPainter painter(this);

    const auto ratio = devicePixelRatioF();
    const QRectF &rf = QRectF(rect());
    const QRectF &rfp = QRectF(m_pixmap.rect());
    QPointF center = rf.center() - rfp.center() / ratio;

    loadSvg();

    painter.drawPixmap(center, m_pixmap);
}

void UosAiWidget::resizeEvent(QResizeEvent *event)
{
    loadSvg();
    update();
    QWidget::resizeEvent(event);
}

void UosAiWidget::mousePressEvent(QMouseEvent *event)
{
    if (containCursorPos()) {
        m_pressed = true;
    } else {
        m_pressed = false;
    }

    update();

    QWidget::mousePressEvent(event);
}

void UosAiWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_pressed = false;
    m_hover = false;
    update();

    QWidget::mouseReleaseEvent(event);
}

void UosAiWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (containCursorPos()) {
        m_hover = true;
    } else {
        m_hover = false;
    }

    QWidget::mouseMoveEvent(event);
}

void UosAiWidget::leaveEvent(QEvent *event)
{
    m_hover = false;
    m_pressed = false;
    update();

    QWidget::leaveEvent(event);
}

bool UosAiWidget::containCursorPos()
{
    QPoint cursorPos = this->mapFromGlobal(QCursor::pos());
    QRect rect(this->rect());

    int iconSize = qMin(rect.width(), rect.height());
    int w = (rect.width() - iconSize) / 2;
    int h = (rect.height() - iconSize) / 2;

    rect = rect.adjusted(w, h, -w, -h);

    return rect.contains(cursorPos);
}

void UosAiWidget::loadSvg()
{
    const auto ratio = devicePixelRatioF();
    const int maxSize = PLUGIN_BACKGROUND_MAX_SIZE - 8;

    const int maxWidth = qMin(maxSize, this->width());
    const int maxHeight = qMin(maxSize, this->height());

    const int maxPixSize = qMin(maxWidth, maxHeight);

    QString icon = ":/assets/icons/deepin/builtin/uosai.svg";
    if(DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType)
        icon = ":/assets/icons/deepin/builtin/uosai_dark.svg";

    QSize iconSize(maxPixSize, maxPixSize);
#ifdef PLUGIN_ICON_MAX_SIZE
    iconSize = QSize(PLUGIN_ICON_MAX_SIZE, PLUGIN_ICON_MAX_SIZE);
#endif
//#ifdef USE_V23_DOCK
    iconSize = QSize(16, 16);
//#endif

    m_pixmap = QPixmap(int(iconSize.width() * ratio), int(iconSize.height() * ratio));
    QSvgRenderer renderer(icon);
    m_pixmap.fill(Qt::transparent);

    QPainter painter;
    painter.begin(&m_pixmap);
    renderer.render(&painter);
    painter.end();
    m_pixmap.setDevicePixelRatio(ratio);
}

QuickPanel::QuickPanel(const QString &desc, QWidget *parent)
{
    QVBoxLayout *lay = new QVBoxLayout;
    lay->setContentsMargins(10, 10, 10, 10);
    lay->setSpacing(0);
    lay->addStretch(1);

    iconLabel = new DLabel;
    iconLabel->setFixedSize(PANEL_ICON_SIZE, PANEL_ICON_SIZE);
    iconLabel->setAlignment(Qt::AlignCenter);
    lay->addWidget(iconLabel, 0, Qt::AlignHCenter);

    DLabel *textLabel = new DLabel;
    textLabel->setText(desc);
    textLabel->setElideMode(Qt::ElideRight);
    textLabel->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(textLabel, DFontSizeManager::T10);
    lay->addSpacing(15);
    lay->addWidget(textLabel, 0, Qt::AlignHCenter);
    lay->addStretch(1);

    setLayout(lay);

    updateIcon();

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &QuickPanel::updateIcon);
}

void QuickPanel::updateIcon()
{
    auto icon = QIcon(QString(":/assets/icons/deepin/builtin/uos-ai-assistant.svg"));
    iconLabel->setPixmap(icon.pixmap(PANEL_ICON_SIZE, PANEL_ICON_SIZE));
    update();
}
