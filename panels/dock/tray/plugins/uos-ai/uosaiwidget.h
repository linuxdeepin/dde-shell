#ifndef UOSAIWIDGET_H
#define UOSAIWIDGET_H

#include <constants.h>

#include <DLabel>

#include <QWidget>
#include <QIcon>

#ifdef DOCK_MIN_SIZE
#define USE_V23_DOCK
#endif

class QLabel;

namespace uos_ai {

class UosAiWidget: public QWidget
{
    Q_OBJECT

public:
    explicit UosAiWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool containCursorPos();
    void loadSvg();
private:
    bool m_hover;
    bool m_pressed;
    QPixmap m_pixmap;
};

class QuickPanel : public QWidget
{
    Q_OBJECT
public:
    explicit QuickPanel(const QString &desc, QWidget *parent = nullptr);
public slots:
    void updateIcon();
private:
    DTK_WIDGET_NAMESPACE::DLabel *iconLabel = nullptr;
};
}
#endif // UOSAIWIDGET_H
