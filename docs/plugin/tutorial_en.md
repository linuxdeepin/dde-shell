# Plugin Development Tutorial

**Version:** 1.0  
**Generated:** 2026-03-07 10:30:00

## Overview

This tutorial will guide you through creating plugins for DDE Shell step by step. We'll cover three types of plugins:
1. QML-based Applet (simplest)
2. Widget-based Applet (intermediate)
3. Containment Plugin (advanced)

Each section includes complete code examples and explanations.

---

## Prerequisites

- CMake 3.16+
- Qt 6 (or Qt 5 with appropriate setup)
- DTK6 (Deepin Toolkit)
- Basic knowledge of C++ and QML
- C++ compiler (GCC or Clang)
- Basic understanding of Qt's plugin system

---

## Tutorial 1: Creating a QML-based Applet

This is the simplest plugin type - a QML-only applet.

### Step 1: Create Directory Structure

```bash
mkdir -p my-qml-applet/package
cd my-qml-applet
```

### Step 2: Create metadata.json

Create `package/metadata.json`:

```json
{
  "Plugin": {
    "Version": "1.0",
    "Id": "org.mycompany.myapplet",
    "Url": "main.qml"
  }
}
```

**Explanation:**
- `Version`: Plugin version number
- `Id`: Unique identifier (use reverse domain notation)
- `Url`: Path to QML file relative to package directory

### Step 3: Create QML File

Create `package/main.qml`:

```qml
// SPDX-FileCopyrightText: 2024 MyCompany
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4
import org.deepin.ds 1.0

AppletItem {
    objectName: "my applet"
    implicitWidth: 100
    implicitHeight: 100
    
    Rectangle {
        anchors.fill: parent
        color: "#2ecc71"
        radius: 8
        
        Text {
            anchors.centerIn: parent
            text: "Hello, World!"
            font.pixelSize: 14
            color: "white"
        }
    }
}
```

**Explanation:**
- `AppletItem`: Base component for QML applets
- `implicitWidth/Height`: Default size
- Rectangle: Visual container
- Text: Display text centered

### Step 4: Create CMakeLists.txt

```cmake
# SPDX-FileCopyrightText: 2024 MyCompany
# SPDX-License-Identifier: GPL-3.0-or-later

ds_install_package(PACKAGE org.mycompany.myapplet)
```

**Explanation:**
- `ds_install_package`: Custom macro to install plugin package
- No C++ code needed for QML-only plugins

### Step 5: Build and Install

```bash
# From dde-shell root
cmake -Bbuild
cmake --build build
cmake --install build
```

### Step 6: Test Your Plugin

After installation, your plugin will be automatically discovered and loaded by DDE Shell.

---

## Tutorial 2: Creating a Widget-based Applet

This plugin uses Qt Widgets for UI.

### Step 1: Create Directory Structure

```bash
mkdir -p my-widget-applet
cd my-widget-applet
```

### Step 2: Create Header File

Create `mywidgetapplet.h`:

```cpp
// SPDX-FileCopyrightText: 2024 MyCompany
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"

DS_USE_NAMESPACE

class MyWidgetApplet : public DApplet
{
    Q_OBJECT
public:
    explicit MyWidgetApplet(QObject *parent = nullptr);
    virtual bool init() override;
};
```

**Explanation:**
- Inherits from `DApplet`
- Implements `init()` method
- Uses `DS_USE_NAMESPACE` for DDE Shell namespace

### Step 3: Create Implementation File

Create `mywidgetapplet.cpp`:

```cpp
// SPDX-FileCopyrightText: 2024 MyCompany
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mywidgetapplet.h"

#include "pluginfactory.h"

#include <DIconButton>
#include <DIconTheme>
#include <DLabel>
#include <dplatformwindowhandle.h>

#include <QHBoxLayout>
#include <QWidget>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

MyWidgetApplet::MyWidgetApplet(QObject *parent)
    : DApplet(parent)
{
}

bool MyWidgetApplet::init()
{
    // Create main widget
    auto widget = new QWidget();
    DPlatformWindowHandle handle(widget);
    widget->setFixedSize(QSize(200, 100));
    
    // Create layout
    auto layout = new QHBoxLayout(widget);
    
    // Create icon button
    auto btn = new DIconButton();
    btn->setIcon(DIconTheme::findQIcon("deepin-home"));
    btn->setIconSize(QSize(32, 32));
    layout->addWidget(btn);
    
    // Create label
    auto label = new DLabel("My Widget");
    label->setStyleSheet("color: white; font-size: 14px;");
    layout->addWidget(label);
    
    // Show widget
    widget->show();
    
    // Call parent init
    return DApplet::init();
}

// Register plugin
D_APPLET_CLASS(MyWidgetApplet)

#include "mywidgetapplet.moc"
```

**Explanation:**
- Creates QWidget with DTK components
- Uses `DPlatformWindowHandle` for window management
- `D_APPLET_CLASS` macro registers plugin
- Must include `.moc` file at the end

### Step 4: Create metadata.json

Create `package/metadata.json`:

```json
{
  "Plugin": {
    "Version": "1.0",
    "Id": "org.mycompany.mywidgetapplet"
  }
}
```

**Note:** No `Url` needed for widget-based plugins.

### Step 5: Create CMakeLists.txt

```cmake
# SPDX-FileCopyrightText: 2024 MyCompany
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Widgets)
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget)

add_library(ds-mywidgetapplet SHARED
    mywidgetapplet.h
    mywidgetapplet.cpp
)

target_link_libraries(ds-mywidgetapplet PRIVATE
    dde-shell-frame
    Qt${QT_VERSION_MAJOR}::Widgets
    Dtk${DTK_VERSION_MAJOR}::Widget
)

ds_install_package(PACKAGE org.mycompany.mywidgetapplet TARGET ds-mywidgetapplet)
```

**Explanation:**
- Builds shared library
- Links against DDE Shell frame, Qt, and DTK
- Installs plugin with `ds_install_package`

### Step 6: Build and Install

```bash
cmake -Bbuild
cmake --build build
cmake --install build
```

---

## Tutorial 3: Creating a Containment Plugin

This is the most complex plugin type - it manages other applets.

### Step 1: Create Directory Structure

```bash
mkdir -p my-containment/package
cd my-containment
```

### Step 2: Create Header File

Create `mycontainment.h`:

```cpp
// SPDX-FileCopyrightText: 2024 MyCompany
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "containment.h"

DS_USE_NAMESPACE

class MyContainment : public DContainment
{
    Q_OBJECT
public:
    explicit MyContainment(QObject *parent = nullptr);
    ~MyContainment();

    virtual bool load() override;
protected:
    virtual QObject *createProxyMeta() override;
};
```

**Explanation:**
- Inherits from `DContainment`
- Implements `load()` and `createProxyMeta()`
- Can manage child applets

### Step 3: Create Implementation File

Create `mycontainment.cpp`:

```cpp
// SPDX-FileCopyrightText: 2024 MyCompany
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mycontainment.h"

#include "pluginfactory.h"
#include "appletproxy.h"
#include <QDebug>

MyContainment::MyContainment(QObject *parent)
    : DContainment(parent)
{
    qDebug() << "MyContainment created";
}

MyContainment::~MyContainment()
{
    qDebug() << "MyContainment destroyed";
}

bool MyContainment::load()
{
    DCORE_USE_NAMESPACE;
    
    // Get all available applets
    auto applets = DPluginLoader::instance()->plugins();
    
    // Filter for specific applets (example)
    QList<DAppletData> groups;
    for (const auto &plugin : applets) {
        if (plugin.pluginId().contains("example")) {
            groups << DAppletData::fromPluginMetaData(plugin);
        }
    }
    
    // Set applet data
    auto data = appletData();
    data.setGroupList(groups);
    setAppletData(data);
    
    // Call parent load
    return DContainment::load();
}

QObject *MyContainment::createProxyMeta()
{
    // You can create a proxy object here for QML communication
    return new QObject(this);
}

// Register plugin
D_APPLET_CLASS(MyContainment)

#include "mycontainment.moc"
```

**Explanation:**
- `load()`: Sets up child applets
- `createProxyMeta()`: Creates proxy for QML
- Uses `DPluginLoader` to discover applets
- Filters and groups applets

### Step 4: Create QML File

Create `package/main.qml`:

```qml
// SPDX-FileCopyrightText: 2024 MyCompany
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import org.deepin.ds 1.0

ContainmentItem {
    id: root
    objectName: "my containment"
    implicitWidth: childrenRect.width
    implicitHeight: childrenRect.height
    
    RowLayout {
        spacing: 10
        
        Repeater {
            model: Containment.appletItems
            delegate: Control {
                contentItem: model.data
                
                Rectangle {
                    width: 80
                    height: 80
                    color: "#3498db"
                    radius: 8
                    
                    Text {
                        anchors.centerIn: parent
                        text: model.data.pluginId
                        color: "white"
                        font.pixelSize: 10
                    }
                }
            }
        }
    }
}
```

**Explanation:**
- `ContainmentItem`: Base for containment UI
- `Repeater`: Iterates over child applets
- `Control`: Delegates to applet content

### Step 5: Create metadata.json

Create `package/metadata.json`:

```json
{
  "Plugin": {
    "Version": "1.0",
    "Id": "org.mycompany.mycontainment",
    "Url": "main.qml",
    "ContainmentType": "Containment"
  }
}
```

**Explanation:**
- `ContainmentType`: "Containment" indicates this is a container
- `Url`: Points to QML file

### Step 6: Create CMakeLists.txt

```cmake
# SPDX-FileCopyrightText: 2024 MyCompany
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(ds-mycontainment SHARED
    mycontainment.h
    mycontainment.cpp
)

target_link_libraries(ds-mycontainment PRIVATE
    dde-shell-frame
)

ds_install_package(PACKAGE org.mycompany.mycontainment TARGET ds-mycontainment)
```

### Step 7: Build and Install

```bash
cmake -Bbuild
cmake --build build
cmake --install build
```

---

## Advanced Topics

### Adding D-Bus Support

To expose functionality via D-Bus:

```cpp
#include <QDBusConnection>

bool MyPlugin::init()
{
    QDBusConnection::sessionBus().registerObject(
        "/org/mycompany/MyPlugin",
        this
    );
    
    return DApplet::init();
}
```

### Handling Configuration

Using DConfig for plugin settings:

```cpp
#include <DConfig>

bool MyPlugin::init()
{
    auto config = DConfig::create("org.mycompany.myplugin");
    QString setting = config->value("mySetting").toString();
    
    return DApplet::init();
}
```

### Signal/Slot Communication

Between C++ and QML:

```cpp
// C++ side
class MyPlugin : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    
signals:
    void statusChanged();
    
public:
    QString status() const { return m_status; }
    
private:
    QString m_status;
};
```

```qml
// QML side
Connections {
    target: root
    function onStatusChanged() {
        console.log("Status changed:", root.status)
    }
}
```

---

## Debugging

### Enable Plugin Logging

```cpp
#include <QDebug>

bool MyPlugin::init()
{
    qDebug() << "MyPlugin::init() called";
    qInfo() << "Plugin ID:" << pluginId();
    return DApplet::init();
}
```

### Common Issues

1. **Plugin not loading**
   - Check `metadata.json` syntax
   - Verify plugin ID is unique
   - Check logs for error messages

2. **QML not showing**
   - Verify `Url` in metadata.json
   - Check QML syntax errors
   - Review QML console output

3. **Crashes on startup**
   - Check for null pointers
   - Verify all resources are available
   - Use debugger to trace

---

## Next Steps

---

## Summary

You've learned how to create:
1. ✅ QML-based applets (simplest)
2. ✅ Widget-based applets (intermediate)
3. ✅ Containment plugins (advanced)

Each tutorial provided complete, working code that you can use as a starting point for your own plugins.

Happy plugin development!
