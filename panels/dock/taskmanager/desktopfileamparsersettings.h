// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QObject>
#include <DConfig>

DCORE_USE_NAMESPACE

DS_BEGIN_NAMESPACE
namespace dock {
class DesktopFileAMParserSettings : QObject
{
    Q_OBJECT

public:
    static DesktopFileAMParserSettings* instance();

    void setDockedAMDektopfileIds(QStringList ids);
    QStringList dockedAMDektopfileIds();

Q_SIGNALS:
    void dockedAMDesktopfileIdsChanged();

private:
    DesktopFileAMParserSettings(QObject* parent = nullptr);
    DConfig *m_dconfig;
};
}
DS_END_NAMESPACE
