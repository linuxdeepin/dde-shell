/**
 * This file is generated by dconfig2cpp.
 * Command line arguments: ./dconfig2cpp -p ./dde-shell/toolGenerate/dconfig2cpp ./dde-shell/panels/dock/taskmanager/dconfig/org.deepin.ds.dock.taskmanager.json
 * Generation time: 2025-01-14T10:55:02
 * JSON file version: 1.0
 * 
 * WARNING: DO NOT MODIFY THIS FILE MANUALLY.
 * If you need to change the content, please modify the dconfig2cpp tool.
 */

#ifndef ORG_DEEPIN_DS_DOCK_TASKMANAGER_H
#define ORG_DEEPIN_DS_DOCK_TASKMANAGER_H

#include <QThread>
#include <QVariant>
#include <QDebug>
#include <QAtomicPointer>
#include <QAtomicInteger>
#include <DConfig>

class org_deepin_ds_dock_taskmanager : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString Allow_Force_Quit READ Allow_Force_Quit WRITE setAllow_Force_Quit NOTIFY Allow_Force_QuitChanged)
    Q_PROPERTY(QList<QVariant> Docked_Items READ Docked_Items WRITE setDocked_Items NOTIFY Docked_ItemsChanged)
    Q_PROPERTY(QString Window_Split READ Window_Split WRITE setWindow_Split NOTIFY Window_SplitChanged)
public:
    explicit org_deepin_ds_dock_taskmanager(QThread *thread, const QString &appId, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(appId, name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    explicit org_deepin_ds_dock_taskmanager(QThread *thread, DTK_CORE_NAMESPACE::DConfigBackend *backend, const QString &appId, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(backend, appId, name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    explicit org_deepin_ds_dock_taskmanager(QThread *thread, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    explicit org_deepin_ds_dock_taskmanager(QThread *thread, DTK_CORE_NAMESPACE::DConfigBackend *backend, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(backend, name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    ~org_deepin_ds_dock_taskmanager() {
        if (m_config.loadRelaxed()) {
            m_config.loadRelaxed()->deleteLater();
        }
    }

    QString Allow_Force_Quit() const {
        return p_Allow_Force_Quit;
    }
    void setAllow_Force_Quit(const QString &value) {
        auto oldValue = p_Allow_Force_Quit;
        p_Allow_Force_Quit = value;
        markPropertySet(0);
        if (auto config = m_config.loadRelaxed()) {
            QMetaObject::invokeMethod(config, [this, value]() {
                m_config.loadRelaxed()->setValue(QStringLiteral("Allow_Force_Quit"), value);
            });
        }
        if (p_Allow_Force_Quit != oldValue) {
            Q_EMIT Allow_Force_QuitChanged();
        }
    }
    QList<QVariant> Docked_Items() const {
        return p_Docked_Items;
    }
    void setDocked_Items(const QList<QVariant> &value) {
        auto oldValue = p_Docked_Items;
        p_Docked_Items = value;
        markPropertySet(1);
        if (auto config = m_config.loadRelaxed()) {
            QMetaObject::invokeMethod(config, [this, value]() {
                m_config.loadRelaxed()->setValue(QStringLiteral("Docked_Items"), value);
            });
        }
        if (p_Docked_Items != oldValue) {
            Q_EMIT Docked_ItemsChanged();
        }
    }
    QString Window_Split() const {
        return p_Window_Split;
    }
    void setWindow_Split(const QString &value) {
        auto oldValue = p_Window_Split;
        p_Window_Split = value;
        markPropertySet(2);
        if (auto config = m_config.loadRelaxed()) {
            QMetaObject::invokeMethod(config, [this, value]() {
                m_config.loadRelaxed()->setValue(QStringLiteral("Window_Split"), value);
            });
        }
        if (p_Window_Split != oldValue) {
            Q_EMIT Window_SplitChanged();
        }
    }
Q_SIGNALS:
    void Allow_Force_QuitChanged();
    void Docked_ItemsChanged();
    void Window_SplitChanged();
private:
    void initialize(DTK_CORE_NAMESPACE::DConfig *config) {
        Q_ASSERT(!m_config.loadRelaxed());
        m_config.storeRelaxed(config);
        if (testPropertySet(0)) {
            config->setValue(QStringLiteral("Allow_Force_Quit"), QVariant::fromValue(p_Allow_Force_Quit));
        } else {
            updateValue(QStringLiteral("Allow_Force_Quit"), QVariant::fromValue(p_Allow_Force_Quit));
        }
        if (testPropertySet(1)) {
            config->setValue(QStringLiteral("Docked_Items"), QVariant::fromValue(p_Docked_Items));
        } else {
            updateValue(QStringLiteral("Docked_Items"), QVariant::fromValue(p_Docked_Items));
        }
        if (testPropertySet(2)) {
            config->setValue(QStringLiteral("Window_Split"), QVariant::fromValue(p_Window_Split));
        } else {
            updateValue(QStringLiteral("Window_Split"), QVariant::fromValue(p_Window_Split));
        }

        connect(config, &DTK_CORE_NAMESPACE::DConfig::valueChanged, this, [this](const QString &key) {
            updateValue(key);
        }, Qt::DirectConnection);
    }
    void updateValue(const QString &key, const QVariant &fallback = QVariant()) {
        Q_ASSERT(QThread::currentThread() == m_config.loadRelaxed()->thread());
        const QVariant &value = m_config.loadRelaxed()->value(key, fallback);
        if (key == QStringLiteral("Allow_Force_Quit")) {
            auto newValue = qvariant_cast<QString>(value);
            QMetaObject::invokeMethod(this, [this, newValue]() {
                if (p_Allow_Force_Quit != newValue) {
                    p_Allow_Force_Quit = newValue;
                    Q_EMIT Allow_Force_QuitChanged();
                }
            });
            return;
        }
        if (key == QStringLiteral("Docked_Items")) {
            auto newValue = qvariant_cast<QList<QVariant>>(value);
            QMetaObject::invokeMethod(this, [this, newValue]() {
                if (p_Docked_Items != newValue) {
                    p_Docked_Items = newValue;
                    Q_EMIT Docked_ItemsChanged();
                }
            });
            return;
        }
        if (key == QStringLiteral("Window_Split")) {
            auto newValue = qvariant_cast<QString>(value);
            QMetaObject::invokeMethod(this, [this, newValue]() {
                if (p_Window_Split != newValue) {
                    p_Window_Split = newValue;
                    Q_EMIT Window_SplitChanged();
                }
            });
            return;
        }
    }
    inline void markPropertySet(const int index) {
        if (index < 32) {
            m_propertySetStatus0.fetchAndOrOrdered(1 << (index - 0));
            return;
        }
        Q_UNREACHABLE();
    }
    inline bool testPropertySet(const int index) const {
        if (index < 32) {
            return (m_propertySetStatus0.loadRelaxed() & (1 << (index - 0)));
        }
        Q_UNREACHABLE();
    }
    QAtomicPointer<DTK_CORE_NAMESPACE::DConfig> m_config = nullptr;
    QString p_Allow_Force_Quit { QStringLiteral("enabled") };
    QList<QVariant> p_Docked_Items { QList<QVariant>{QVariant(QStringLiteral("id: dde-file-manager,type: amAPP")), QVariant(QStringLiteral("id: deepin-app-store,type: amAPP")), QVariant(QStringLiteral("id: org.deepin.browser,type: amAPP")), QVariant(QStringLiteral("id: deepin-mail,type: amAPP")), QVariant(QStringLiteral("id: deepin-terminal,type: amAPP")), QVariant(QStringLiteral("id: dde-calendar,type: amAPP")), QVariant(QStringLiteral("id: deepin-music,type: amAPP")), QVariant(QStringLiteral("id: deepin-editor,type: amAPP")), QVariant(QStringLiteral("id: deepin-calculator,type: amAPP")), QVariant(QStringLiteral("id: org.deepin.dde.control-center, type: amAPP")), QVariant(QStringLiteral("id: dde-trash,type: amAPP"))} };
    QString p_Window_Split { QStringLiteral("disabled") };
    QAtomicInteger<quint32> m_propertySetStatus0 = 0;
};

#endif // ORG_DEEPIN_DS_DOCK_TASKMANAGER_H