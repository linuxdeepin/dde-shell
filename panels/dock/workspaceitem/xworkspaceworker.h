// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef XWORKSPACEWORKER_H
#define XWORKSPACEWORKER_H

#include "dsglobal.h"

#include <QObject>
#include <QDBusInterface>
DS_BEGIN_NAMESPACE
namespace dock {
class WorkspaceModel;
class XWorkspaceWorker : public QObject
{
    Q_OBJECT
public:
    explicit XWorkspaceWorker(WorkspaceModel *model);

public slots:
    void updateData();
    void setIndex(int index);
signals:

private:
    QDBusInterface *m_inter;
    WorkspaceModel *m_model;
};
}
DS_END_NAMESPACE
#endif // XWORKSPACEWORKER_H
