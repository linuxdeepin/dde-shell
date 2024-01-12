// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsglobal.h"
#include "docksettingsdconfig.h"
#include "dockabstractsettingsconfig.h"

#include <QObject>
#include <DConfig>
#include <QVariant>

DS_BEGIN_NAMESPACE
namespace dock {
DockDconfig::DockDconfig(QObject* parent)
    : DockAbstractConfig(parent)
    , m_dconfig(DConfig::create("org.deepin.ds.dock", "org.deepin.ds.dock", QString(), this))
{
    connect(m_dconfig.data(), &DConfig::valueChanged, this, &DockAbstractConfig::valueChanged);
}

QVariant DockDconfig::value(const QString &key)
{
    if (isValid())
        return m_dconfig->value(key);
    return QVariant();
}

void DockDconfig::setValue(const QString &key, const QVariant &value)
{
    if (isValid())
        m_dconfig->setValue(key, value);
}

bool DockDconfig::isValid()
{
    return m_dconfig && m_dconfig->isValid();
}

}
DS_END_NAMESPACE
