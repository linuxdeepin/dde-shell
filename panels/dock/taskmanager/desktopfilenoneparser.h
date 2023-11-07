// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "desktopfileparserfactory.h"
#include "desktopfileabstractparser.h"

#include <QObject>

DS_BEGIN_NAMESPACE

namespace dock {
class AbstractWindow;

class DesktopFileNoneParser : public DesktopfileAbstractParser
{
    Q_OBJECT
public:
    ~DesktopFileNoneParser();

    virtual void launch() override;
    virtual void launchWithAction(const QString& action) override;
    virtual void requestQuit() override;

    virtual QString id() override;
    virtual QString name() override;
    virtual QList<QPair<QString, QString>> actions() override;
    virtual QString desktopIcon() override;
    virtual QString genericName() override;
    virtual bool isDocked() override;
    virtual bool isValied() override;
    virtual void setDocked(bool docked) override;

private:
    friend class DesktopfileParserFactory<DesktopFileNoneParser>;
    DesktopFileNoneParser(QString id, QObject *parent = nullptr);

    static QString identifyWindow(QPointer<AbstractWindow> window);

private:
    QString m_id;

};
}
DS_END_NAMESPACE
