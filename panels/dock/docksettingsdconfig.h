// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "dockabstractsettingsconfig.h"

#include <DConfig>

#include <QVariant>
#include <QScopedPointer>

DCORE_USE_NAMESPACE

DS_BEGIN_NAMESPACE
namespace dock {
class DockDconfig : public DockAbstractConfig
{
    Q_OBJECT
public:
    DockDconfig(QObject* parent = nullptr);
    virtual QVariant value(const QString& key) override;
    virtual void setValue(const QString& key, const QVariant& value) override;
    virtual bool isValid() override;

private:
    QScopedPointer<DConfig> m_dconfig;
};
}
DS_END_NAMESPACE
