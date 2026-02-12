// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractItemModel>
#include <QFont>
#include <QtQml/QtQml>

namespace dock
{
class TextCalculator;
class TextCalculatorAttached : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString elidedText READ elidedText NOTIFY elidedTextChanged)
    Q_PROPERTY(qreal ellipsisWidth READ ellipsisWidth NOTIFY ellipsisWidthChanged)
    Q_PROPERTY(bool isTruncated READ isTruncated NOTIFY isTruncatedChanged)
    Q_PROPERTY(TextCalculator *calculator READ calculator NOTIFY calculatorChanged)

public:
    explicit TextCalculatorAttached(QObject *parent = nullptr);
    ~TextCalculatorAttached();

    void setCalculator(TextCalculator *calculator);
    TextCalculator *calculator();

    QString text() const
    {
        return m_text;
    }
    void setText(const QString &text);

    QString elidedText() const;

    qreal ellipsisWidth() const
    {
        return m_ellipsisWidth;
    }

    bool isTruncated() const
    {
        return m_isTruncated;
    }

    void ensureInitialize();

Q_SIGNALS:
    void textChanged();
    void elidedTextChanged();
    void ellipsisWidthChanged();
    void isTruncatedChanged();
    void calculatorChanged();

private Q_SLOTS:
    void updateElidedText();

private:
    QString m_text;
    QString m_elidedText;
    qreal m_ellipsisWidth = 0.0;
    bool m_isTruncated = false;
    TextCalculator *m_calculator;
    bool m_initialized = false;
};

class TextCalculator : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT
    QML_ATTACHED(TextCalculatorAttached)
    Q_PROPERTY(qreal optimalSingleTextWidth READ optimalSingleTextWidth NOTIFY optimalSingleTextWidthChanged)
    Q_PROPERTY(qreal totalWidth READ totalWidth NOTIFY totalWidthChanged)
    Q_PROPERTY(QAbstractItemModel *dataModel READ dataModel WRITE setDataModel NOTIFY dataModelChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(qreal remainingSpace READ remainingSpace WRITE setRemainingSpace NOTIFY remainingSpaceChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(qreal iconSize READ iconSize WRITE setIconSize NOTIFY iconSizeChanged)
    Q_PROPERTY(qreal cellSize READ cellSize WRITE setCellSize NOTIFY cellSizeChanged)
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(int itemPadding READ itemPadding WRITE setItemPadding NOTIFY itemPaddingChanged)

public:
    explicit TextCalculator(QObject *parent = nullptr);
    ~TextCalculator();

    qreal optimalSingleTextWidth() const
    {
        return m_optimalSingleTextWidth;
    }

    qreal totalWidth() const
    {
        return m_totalWidth;
    }

    QFont font() const
    {
        return m_font;
    }
    void setFont(const QFont &font);

    qreal iconSize() const
    {
        return m_iconSize;
    }
    void setIconSize(qreal size);

    qreal cellSize() const
    {
        return m_cellSize;
    }
    void setCellSize(qreal size);

    int spacing() const
    {
        return m_spacing;
    }
    void setSpacing(int spacing);

    int itemPadding() const
    {
        return m_itemPadding;
    }
    void setItemPadding(int padding);

    QAbstractItemModel *dataModel() const
    {
        return m_dataModel;
    }
    void setDataModel(QAbstractItemModel *model);

    qreal remainingSpace() const
    {
        return m_remainingSpace;
    }
    void setRemainingSpace(qreal space);

    bool isEnabled() const
    {
        return m_enabled;
    }
    void setEnabled(bool enabled);

    static TextCalculatorAttached *qmlAttachedProperties(QObject *object);

    virtual void classBegin() override
    {
    }
    virtual void componentComplete() override;

Q_SIGNALS:
    void optimalSingleTextWidthChanged();
    void totalWidthChanged();
    void fontChanged();
    void iconSizeChanged();
    void cellSizeChanged();
    void spacingChanged();
    void itemPaddingChanged();
    void dataModelChanged();
    void remainingSpaceChanged();
    void enabledChanged();

private slots:
    void onDataModelChanged();
    void calculateOptimalTextWidth();

private:
    void connectDataModelSignals();
    void disconnectDataModelSignals();
    void scheduleCalculation();

    qreal calculateBaselineWidth(int charCount) const;
    qreal calculateElidedTextWidth(const QString &text, qreal maxWidth) const;
    QStringList getApplicationTitles() const;

    bool complete = false;
    qreal m_optimalSingleTextWidth;
    qreal m_totalWidth;
    QFont m_font;
    qreal m_iconSize;
    qreal m_cellSize;
    int m_spacing;
    int m_itemPadding;
    QAbstractItemModel *m_dataModel;
    qreal m_remainingSpace;
    bool m_enabled;

    QHash<int, qreal> m_baselineWidthCache; // Cache for baseline widths of different character counts
};

}
QML_DECLARE_TYPEINFO(dock::TextCalculator, QML_HAS_ATTACHED_PROPERTIES)
