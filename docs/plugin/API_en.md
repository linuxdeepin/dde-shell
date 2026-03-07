# DDE Shell Plugin API Reference

**Version:** 1.0  
**Generated:** 2026-03-07 10:30:00

## Overview

DDE Shell uses a plugin-based architecture where panels and applets can be loaded dynamically at runtime. Plugins are discovered via `metadata.json` files and loaded using Qt's plugin system.

## Plugin Types

### 1. Applet Plugin

Base class for single-function plugins (widgets, indicators, etc.).

**Base Class:** `DApplet` (frame/applet.h)

**Required Methods:**
```cpp
virtual bool load() override;
virtual bool init() override;
```

**Key Properties:**
- `id`: Unique instance identifier
- `pluginId`: Plugin identifier from metadata.json
- `parent`: Parent applet (for containment)
- `rootObject`: QML root object (for QML plugins)

**Subtypes:**

#### a. QML-based Applet
- Uses QML for UI
- metadata.json specifies `Url` to QML file
- Automatically loads QML and creates root object

#### b. Widget-based Applet
- Uses Qt Widgets (QWidget) for UI
- Manually creates widgets in `init()`
- Uses `DPlatformWindowHandle` for window management

### 2. Containment Plugin

Container plugin that manages multiple child applets.

**Base Class:** `DContainment` (frame/containment.h)

**Required Methods:**
```cpp
virtual bool load() override;
virtual bool init() override;
DApplet *createApplet(const DAppletData &data);
void removeApplet(DApplet *applet);
```

**Key Properties:**
- `appletItems`: Model of child applets for QML
- `applets()`: List of child applet instances

**Protected Methods:**
```cpp
virtual QObject *createProxyMeta() override;
```

**Use Case:** Dock panel, notification center - containers for multiple items.

### 3. Panel Plugin

Specialized containment for dock/notification panels.

**Base Class:** `DPanel` (inherits from `DContainment`)

**Required Methods:**
```cpp
virtual bool load() override;
virtual bool init() override;
```

**Use Case:** Specific panel implementations (dock, notification).

## Plugin Metadata

### metadata.json Format

All plugins require a `package/metadata.json` file:

```json
{
  "Plugin": {
    "Version": "1.0",
    "Id": "org.deepin.ds.example.applet",
    "Url": "main.qml",
    "Parent": "org.deepin.ds.example.containment",
    "ContainmentType": "Panel|Containment|Applet"
  }
}
```

### Metadata Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `Version` | String | Yes | Plugin version (e.g., "1.0") |
| `Id` | String | Yes | Unique plugin identifier (reverse domain style) |
| `Url` | String | Conditional* | QML file path (required for QML plugins) |
| `Parent` | String | Conditional* | Parent plugin ID (for child plugins) |
| `ContainmentType` | String | Conditional* | Plugin type: "Panel", "Containment", or "Applet" |

*Conditional based on plugin type

### ID Naming Convention

Use reverse domain notation: `org.deepin.ds.<component>.<name>`

Examples:
- `org.deepin.ds.dock.taskmanager`
- `org.deepin.ds.notification.center`
- `org.deepin.ds.example.applet`

## Plugin Registration

### C++ Class Registration

Use `D_APPLET_CLASS` macro to register plugin class:

```cpp
#include "pluginfactory.h"

// Your plugin class
class MyPlugin : public DApplet
{
    Q_OBJECT
public:
    explicit MyPlugin(QObject *parent = nullptr);
    virtual bool init() override;
};

// Register the plugin
D_APPLET_CLASS(MyPlugin)

#include "myplugin.moc"
```

### Build Configuration

#### CMakeLists.txt for Applets

```cmake
# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Widgets)
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget)

add_library(ds-my-plugin SHARED
    myplugin.h
    myplugin.cpp
)

target_link_libraries(ds-my-plugin PRIVATE
    dde-shell-frame
    Qt${QT_VERSION_MAJOR}::Widgets
    Dtk${DTK_VERSION_MAJOR}::Widget
)

ds_install_package(PACKAGE org.deepin.ds.myplugin TARGET ds-my-plugin)
```

#### CMakeLists.txt for QML-only Plugins

```cmake
# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later

ds_install_package(PACKAGE org.deepin.ds.myplugin)
```

## Core APIs

### DApplet API

```cpp
class DS_SHARE DApplet : public QObject, public DTK_CORE_NAMESPACE::DObject
{
    Q_PROPERTY(QString id READ id CONSTANT FINAL)
    Q_PROPERTY(QString pluginId READ pluginId CONSTANT FINAL)
    Q_PROPERTY(QObject *rootObject READ rootObject NOTIFY rootObjectChanged)

    // Lifecycle
    virtual bool load() override;
    virtual bool init() override;

    // Properties
    QString id() const;
    QString pluginId() const;
    QObject *rootObject() const;
    DApplet *parentApplet() const;

    // Metadata
    DPluginMetaData pluginMetaData() const;
    DAppletData appletData() const;
    void setAppletData(const DAppletData &data);
    void setRootObject(QObject *root);

signals:
    void rootObjectChanged();

protected:
    // Create proxy object for QML communication
    virtual QObject *createProxyMeta();
};
```

### DContainment API

```cpp
class DS_SHARE DContainment : public DApplet
{
    Q_PROPERTY(DAppletItemModel *appletItems READ appletItemModel CONSTANT)

    // Applet management
    DApplet *createApplet(const DAppletData &data);
    void removeApplet(DApplet *applet);
    QList<DApplet *> applets() const;
    QList<QObject *> appletItems();
    DAppletItemModel *appletItemModel() const;
    DApplet *applet(const QString &id) const;

    // Lifecycle (inherits from DApplet)
    virtual bool load() override;
    virtual bool init() override;

protected:
    virtual QObject *createProxyMeta() override;
};
```

### DPluginMetaData API

```cpp
class DS_SHARE DPluginMetaData : public QObject
{
    // Validation
    bool isValid() const;

    // Accessors
    QString pluginId() const;
    QString pluginDir() const;
    QString url() const;
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

    // Static creators
    static DPluginMetaData fromJsonFile(const QString &file);
    static DPluginMetaData fromJsonString(const QByteArray &data);
    static DPluginMetaData rootPluginMetaData();
    static bool isRootPlugin(const QString &pluginId);
};
```

### DAppletBridge API

Bridge for communicating with applet from C++:

```cpp
#include "appletbridge.h"

// Create bridge to applet
DAppletBridge bridge("org.deepin.ds.example.applet");

// Check if applet is available
if (auto applet = bridge.applet()) {
    // Get property
    QString value = applet->property("mainText").toString();

    // Invoke method
    QMetaObject::invokeMethod(applet, "call", Qt::DirectConnection,
                              Q_RETURN_ARG(QString, result),
                              Q_ARG(const QString&, id));

    // Connect to signals
    QObject::connect(applet, SIGNAL(sendSignal(const QString&)),
                     this, SLOT(onReceivedSignal(const QString&)));
}
```

## QML Integration

### QML Imports

Plugins can import DDE Shell QML modules:

```qml
import QtQuick 2.11
import org.deepin.ds 1.0
```

### Available QML Components

#### AppletItem
Base component for QML applets:

```qml
AppletItem {
    implicitWidth: 100
    implicitHeight: 100
    Rectangle {
        anchors.fill: parent
        // Your content here
    }
}
```

#### ContainmentItem
Base component for containment plugins:

```qml
ContainmentItem {
    implicitWidth: childrenRect.width
    implicitHeight: childrenRect.height
    RowLayout {
        Repeater {
            model: Containment.appletItems
            delegate: Control {
                contentItem: model.data
            }
        }
    }
}
```

## Plugin Lifecycle

### Loading Process

1. **Discovery**: DPluginLoader scans plugin directories for `metadata.json`
2. **Parsing**: DPluginMetaData parses metadata from JSON
3. **Instantiation**: DAppletFactory creates plugin instance via Qt plugin system
4. **load()**: Plugin's `load()` method is called
5. **init()**: Plugin's `init()` method is called
6. **QML Loading** (if applicable): QML file is loaded and root object created

### Unloading Process

1. **Shutdown**: Plugin cleanup is handled
2. **Removal**: Applet is removed from parent containment
3. **Resource Cleanup**: All resources are released

## Best Practices

### 1. Plugin IDs
- Use reverse domain notation
- Follow pattern: `org.deepin.ds.<component>.<name>`
- Keep IDs unique across all plugins

### 2. Error Handling
```cpp
bool MyPlugin::init()
{
    // Check for required resources
    if (!requiredResourceAvailable()) {
        qWarning() << "Required resource not available";
        return false;
    }

    // Call parent init
    return DApplet::init();
}
```

### 3. QML Performance
- Use properties efficiently (avoid excessive recalculations)
- Cache expensive operations
- Use Connections for reactive updates

### 4. Memory Management
- Use Qt's parent-child system
- Let Qt handle cleanup where possible
- Be careful with explicit `delete` operations

### 5. Proxy Objects
- Use `createProxyMeta()` to expose methods to QML
- Mark methods with `Q_INVOKABLE`
- Use signals/properties for reactive communication

## Examples

See the `example/` directory for complete plugin examples:
- `applet-example`: QML-based applet
- `applet-widget-example`: Widget-based applet
- `containment-example`: Containment plugin
- `panel-example`: Panel plugin

## Troubleshooting

### Plugin Not Loading
1. Check `metadata.json` is valid JSON
2. Verify plugin ID is unique
3. Check plugin library is built and installed
4. Review logs for error messages

### QML Not Loading
1. Verify `Url` in metadata.json points to existing QML file
2. Check QML syntax is correct
3. Ensure all required imports are present
4. Review QML console logs

### Plugin Crashes
1. Check null pointer accesses
2. Verify all required resources are available
3. Use debugger to trace crashes
4. Review plugin lifecycle methods

## See Also

- [Plugin Tutorial](./tutorial.md) - Step-by-step guide to creating plugins
- [Example Plugins](../../example/) - Complete working examples
