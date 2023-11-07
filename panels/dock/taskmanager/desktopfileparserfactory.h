// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "abstractwindow.h"

#include <QMap>
#include <QObject>
#include <QPointer>

DS_BEGIN_NAMESPACE
namespace dock {
class DesktopfileAbstractParser;

template<typename T>
class DesktopfileParserFactory
{
    static_assert(std::is_base_of<DesktopfileAbstractParser, T>::value, "T must be a subclass of DesktopfileAbstractParser");

public:
    [[nodiscard]] static QSharedPointer<DesktopfileAbstractParser> createByWindow(QPointer<AbstractWindow> window)
    {
        auto id = identifyWindow(window);
        if (id.isEmpty()) return nullptr;
        return createById(id);
    }

    [[nodiscard]] static QSharedPointer<DesktopfileAbstractParser> createById(const QString& id)
    {
        auto desktopFileParser = m_desktopFileParsers.value(id, QWeakPointer<T>(nullptr)).toStrongRef();
        if (desktopFileParser.isNull()) {
            desktopFileParser = QSharedPointer<T>(new T(id));
            m_desktopFileParsers.insert(id, desktopFileParser.toWeakRef());
        }
        return desktopFileParser;
    }

private:
    static QString identifyWindow(QPointer<AbstractWindow> window)
    {
        return T::identifyWindow(window);
    }

private:
    static inline QMap<QString, QWeakPointer<T>> m_desktopFileParsers;
};
}
DS_END_NAMESPACE
