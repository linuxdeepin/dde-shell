// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "textcalculator.h"

#include <QFontMetricsF>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQuickItem>

namespace dock
{
Q_LOGGING_CATEGORY(textCalculatorLog, "ds.taskmanager.textcalculator");

static bool isValidElidedText(const QString &text)
{
    return !text.isEmpty() && text != "…";
}

TextCalculator::TextCalculator(QObject *parent)
    : QObject(parent)
    , m_optimalSingleTextWidth(0.0)
    , m_totalWidth(0)
    , m_font(QGuiApplication::font())
    , m_iconSize(48)
    , m_cellSize(48)
    , m_spacing(8)
    , m_itemPadding(4)
    , m_dataModel(nullptr)
    , m_remainingSpace(0)
    , m_enabled(false)
{
}

TextCalculator::~TextCalculator()
{
    if (m_dataModel) {
        disconnectDataModelSignals();
    }
}

void TextCalculator::setFont(const QFont &font)
{
    if (m_font != font) {
        qCDebug(textCalculatorLog) << "Font changed, clearing cache and recalculating";
        m_font = font;
        m_baselineWidthCache.clear();
        emit fontChanged();
        scheduleCalculation();
    }
}

void TextCalculator::setIconSize(qreal size)
{
    if (m_iconSize != size) {
        m_iconSize = size;
        emit iconSizeChanged();
        scheduleCalculation();
    }
}

void TextCalculator::setCellSize(qreal size)
{
    if (m_cellSize != size) {
        m_cellSize = size;
        emit cellSizeChanged();
        scheduleCalculation();
    }
}

void TextCalculator::setSpacing(int spacing)
{
    if (m_spacing != spacing) {
        m_spacing = spacing;
        emit spacingChanged();
        scheduleCalculation();
    }
}

void TextCalculator::setItemPadding(int padding)
{
    if (m_itemPadding != padding) {
        m_itemPadding = padding;
        emit itemPaddingChanged();
        scheduleCalculation();
    }
}

void TextCalculator::setDataModel(QAbstractItemModel *model)
{
    if (m_dataModel != model) {
        qCDebug(textCalculatorLog) << "DataModel changed, reconnecting signals";
        disconnectDataModelSignals();
        m_dataModel = model;
        connectDataModelSignals();
        emit dataModelChanged();
        scheduleCalculation();
    }
}

void TextCalculator::setRemainingSpace(qreal space)
{
    if (m_remainingSpace != space) {
        m_remainingSpace = space;
        emit remainingSpaceChanged();
        scheduleCalculation();
    }
}

void TextCalculator::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    qCDebug(textCalculatorLog) << "TextCalculator enabled state changed to:" << enabled;
    m_enabled = enabled;
    if (m_enabled) {
        scheduleCalculation();
    } else {
        m_optimalSingleTextWidth = 0.0;
        m_totalWidth = 0;
        emit optimalSingleTextWidthChanged();
        emit totalWidthChanged();
    }
    emit enabledChanged();
}

void TextCalculator::componentComplete()
{
    complete = true;
    scheduleCalculation();
}

void TextCalculator::connectDataModelSignals()
{
    if (m_dataModel) {
        connect(m_dataModel, &QAbstractItemModel::rowsInserted, this, &TextCalculator::onDataModelChanged);
        connect(m_dataModel, &QAbstractItemModel::rowsRemoved, this, &TextCalculator::onDataModelChanged);
        connect(m_dataModel,
                &QAbstractItemModel::dataChanged,
                this,
                [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles) {
                    const auto titleRole = m_dataModel->roleNames().key("title");
                    if (roles.contains(titleRole) || roles.isEmpty()) {
                        scheduleCalculation();
                    }
                });
        connect(m_dataModel, &QAbstractItemModel::modelReset, this, &TextCalculator::onDataModelChanged);
    }
}

void TextCalculator::disconnectDataModelSignals()
{
    if (m_dataModel) {
        disconnect(m_dataModel, nullptr, this, nullptr);
    }
}

void TextCalculator::onDataModelChanged()
{
    scheduleCalculation();
}

void TextCalculator::scheduleCalculation()
{
    if (!complete)
        return;

    if (!m_enabled || !m_dataModel) {
        return;
    }
    calculateOptimalTextWidth();
}

qreal TextCalculator::calculateBaselineWidth(int charCount) const
{
    if (m_baselineWidthCache.contains(charCount)) {
        return m_baselineWidthCache[charCount];
    }

    // Generate baseline text: repeat "字" × character count
    QString baselineText = QString("字").repeated(charCount);

    QFontMetricsF fontMetrics(m_font);
    qreal width = fontMetrics.horizontalAdvance(baselineText);

    const_cast<TextCalculator *>(this)->m_baselineWidthCache[charCount] = width;
    return width;
}

qreal TextCalculator::calculateElidedTextWidth(const QString &text, qreal maxWidth) const
{
    if (text.isEmpty()) {
        return 0.0;
    }

    QFontMetricsF fontMetrics(m_font);
    QString elidedText = fontMetrics.elidedText(text, Qt::ElideRight, maxWidth);
    if (!isValidElidedText(elidedText))
        return 0.0;

    return fontMetrics.horizontalAdvance(elidedText);
}

QStringList TextCalculator::getApplicationTitles() const
{
    QStringList titles;

    if (!m_dataModel) {
        return titles;
    }

    const int rowCount = m_dataModel->rowCount();

    for (int i = 0; i < rowCount; ++i) {
        QModelIndex index = m_dataModel->index(i, 0);

        QString title;

        QHash<int, QByteArray> roleNames = m_dataModel->roleNames();

        // Find title-related role
        for (auto it = roleNames.begin(); it != roleNames.end(); ++it) {
            if (it.value() == "title") {
                QVariant titleData = m_dataModel->data(index, it.key());
                if (titleData.isValid() && !titleData.toString().isEmpty()) {
                    title = titleData.toString();
                    break;
                }
            }
        }

        // If title is empty, keep it as empty string (indicating icon-only display)
        titles.append(title);
    }

    return titles;
}

void TextCalculator::calculateOptimalTextWidth()
{
    QStringList titles = getApplicationTitles();
    const int appCount = titles.size();

    if (appCount <= 0 || m_remainingSpace <= 0) {
        if (m_optimalSingleTextWidth != 0.0) {
            qCDebug(textCalculatorLog) << "Setting optimal width to 0 (no apps or no space)";
            m_optimalSingleTextWidth = 0.0;
            m_totalWidth = 0;
            emit optimalSingleTextWidthChanged();
            emit totalWidthChanged();
        }
        return;
    }

    qreal newOptimalWidth = 0.0;
    qreal newTotalWidth = 0.0;
    int charCount = 7; // Maximum character count limit

    // Iterate from 7 characters to 2 characters, finding the optimal solution
    for (; charCount >= 2; --charCount) {
        // 1. Calculate baseline width (based on character count)
        qreal baselineWidth = calculateBaselineWidth(charCount);

        // 2. Calculate total width for each app item: icon + padding + text
        qreal totalRequiredWidth = 0.0;

        for (int i = 0; i < titles.size(); ++i) {
            const QString &title = titles[i];
            // Base width for each app item = icon width
            qreal itemWidth = m_iconSize;

            qreal textWidth = calculateElidedTextWidth(title, baselineWidth);
            // Only add spacing between icon and text when text is present
            if (textWidth > 0.0) {
                qreal appTitleSpacing = qMax(10.0, m_iconSize / 3.0);
                itemWidth = m_iconSize + m_itemPadding + textWidth + appTitleSpacing;
            } else {
                itemWidth = m_cellSize;
            }

            totalRequiredWidth += itemWidth;
        }

        // 3. Add spacing between apps
        qreal spacingWidth = m_spacing * qMax(0, appCount - 1);
        qreal totalSpaceRequired = totalRequiredWidth + spacingWidth;

        // 4. Check if space requirements are met
        if (totalSpaceRequired <= m_remainingSpace) {
            newOptimalWidth = baselineWidth;
            newTotalWidth = totalSpaceRequired;
            break;
        }
    }

    // Update results
    if (!qFuzzyCompare(m_optimalSingleTextWidth, newOptimalWidth)) {
        qCDebug(textCalculatorLog) << "Optimal text width changed from" << m_optimalSingleTextWidth << "to" << newOptimalWidth << "App count:" << appCount
                                   << "Remaining space:" << m_remainingSpace << "Total required:" << newTotalWidth << "Char count:" << charCount
                                   << "spacing:" << m_spacing;
        m_optimalSingleTextWidth = newOptimalWidth;
        emit optimalSingleTextWidthChanged();
        m_totalWidth = newTotalWidth;
        emit totalWidthChanged();
    }
}

TextCalculatorAttached *TextCalculator::qmlAttachedProperties(QObject *object)
{
    return new TextCalculatorAttached(object);
}

TextCalculatorAttached::TextCalculatorAttached(QObject *parent)
    : QObject(parent)
    , m_calculator(nullptr)
    , m_initialized(false)
{
    connect(this, &TextCalculatorAttached::textChanged, this, &TextCalculatorAttached::updateElidedText);
}

TextCalculatorAttached::~TextCalculatorAttached()
{
}

static TextCalculator *findCalculatorForObject(QObject *object)
{
    if (!object) {
        qCDebug(textCalculatorLog) << "findCalculatorForObject: null object";
        return nullptr;
    }

    QQuickItem *obj = qobject_cast<QQuickItem *>(object);

    // Traverse up parent objects to find TextCalculator instance
    while (obj) {
        // Check if current object is a TextCalculator
        if (auto *calculator = qobject_cast<TextCalculator *>(obj)) {
            return calculator;
        }

        // Check if current object's children contain a TextCalculator
        if (auto calculator = obj->findChild<TextCalculator *>(Qt::FindDirectChildrenOnly)) {
            return calculator;
        }

        obj = obj->parentItem();
    }

    qCWarning(textCalculatorLog) << "No TextCalculator found for object";
    return nullptr;
}

void TextCalculatorAttached::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    ensureInitialize();
    emit textChanged();
}

QString TextCalculatorAttached::elidedText() const
{
    const_cast<TextCalculatorAttached *>(this)->ensureInitialize();
    return m_elidedText;
}

void TextCalculatorAttached::setCalculator(TextCalculator *calculator)
{
    if (calculator) {
        m_calculator = calculator;
        connect(calculator, &TextCalculator::optimalSingleTextWidthChanged, this, &TextCalculatorAttached::updateElidedText);
        updateElidedText();
    }
}

TextCalculator *TextCalculatorAttached::calculator()
{
    ensureInitialize();
    return m_calculator;
}

void TextCalculatorAttached::ensureInitialize()
{
    if (m_initialized) {
        return;
    }

    m_initialized = true;
    if (!m_calculator) {
        auto calculator = findCalculatorForObject(parent());
        setCalculator(calculator);
    }
}

void TextCalculatorAttached::updateElidedText()
{
    if (!m_calculator) {
        qCDebug(textCalculatorLog) << "No calculator available for elided text update";
        m_elidedText.clear();
        m_ellipsisWidth = 0.0;
        m_isTruncated = false;
        emit elidedTextChanged();
        emit ellipsisWidthChanged();
        emit isTruncatedChanged();
        return;
    }

    QFontMetricsF fontMetrics(m_calculator->font());
    qreal maxWidth = m_calculator->optimalSingleTextWidth();

    QString newElidedText = fontMetrics.elidedText(m_text, Qt::ElideRight, maxWidth);
    if (!isValidElidedText(newElidedText)) {
        newElidedText = {};
    } else {
        m_isTruncated = (m_text != newElidedText);
        emit isTruncatedChanged();
        newElidedText.replace(QString::fromUtf8("…"), "");
    }
    
    if (m_isTruncated) {
        m_ellipsisWidth = fontMetrics.horizontalAdvance(QString::fromUtf8("…"));
        emit ellipsisWidthChanged();
    }
        
    if (m_elidedText != newElidedText) {
        m_elidedText = newElidedText;
        emit elidedTextChanged();
    }
}

} // namespace dock
