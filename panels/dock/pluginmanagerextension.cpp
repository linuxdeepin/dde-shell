// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pluginmanagerextension_p.h"
#include "pluginmanagerintegration_p.h"
#include "constants.h"

#include <DGuiApplicationHelper>
#include <DPlatformTheme>

#include <cstdint>

#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandView>
#include <QtWaylandCompositor/QWaylandTextInputManager>
#include <QtWaylandCompositor/QWaylandTextInputManagerV3>
#include <QtWaylandCompositor/QWaylandQtTextInputMethodManager>
#include <QtWaylandCompositor/QWaylandTextInput>
#include <QtWaylandCompositor/QWaylandTextInputV3>
#include <private/qwaylandtextinput_p.h>
#include <private/qwaylandtextinputv3_p.h>
#include <private/qwaylandqttextinputmethod_p.h>
#include <QtWaylandCompositor/QWaylandQtTextInputMethod>
#include <QtWaylandCompositor/QWaylandQuickItem>

#include <QGuiApplication>
#include <QInputMethod>
#include <QClipboard>
#include <QMimeData>

#include <QJsonObject>
#include <QJsonParseError>

#define protected public
#include <private/qwaylandcompositor_p.h>
#undef protected
#include <qpa/qwindowsysteminterface_p.h>

#ifdef USE_DEEPIN_QT
#include <private/qwlqtkey_p.h>
#include <private/qwlqttouch_p.h>
#endif

DGUI_USE_NAMESPACE

struct WlQtTextInputMethodHelper : public QtWaylandServer::qt_text_input_method_v1 {
    static void setFocusCustom(QWaylandQtTextInputMethodPrivate *d, QWaylandSurface *surface) {
        auto *base = static_cast<QtWaylandServer::qt_text_input_method_v1*>(d);
        if (d->focusedSurface == surface) return;

        QtWaylandServer::qt_text_input_method_v1::Resource *newResource = nullptr;
        if (surface && surface->client()) {
            auto resources = base->resourceMap().values(surface->client()->client());
            if (!resources.isEmpty()) {
                newResource = resources.first();
            }
        }

        if (d->focusedSurface) {
            if (d->resource) {
                base->send_leave(d->resource->handle, d->focusedSurface->resource());
            }
            d->focusDestroyListener.reset();
        }

        d->focusedSurface = surface;
        d->resource = newResource;

        if (d->resource != nullptr && d->focusedSurface != nullptr) {
            d->surroundingText.clear();
            d->cursorPosition = 0;
            d->anchorPosition = 0;
            d->absolutePosition = 0;
            d->cursorRectangle = QRect();
            d->preferredLanguage.clear();
            d->hints = Qt::InputMethodHints();
            base->send_enter(d->resource->handle, d->focusedSurface->resource());
            base->send_input_direction_changed(d->resource->handle, int(qApp->inputMethod()->inputDirection()));
            base->send_locale_changed(d->resource->handle, qApp->inputMethod()->locale().bcp47Name());

            d->focusDestroyListener.listenForDestruction(surface->resource());
            if (d->inputPanelVisible && d->enabledSurfaces.values().contains(surface))
                qApp->inputMethod()->show();
        }
    }
};

PluginScaleManager::PluginScaleManager(QWaylandCompositor *compositor)
    : QWaylandCompositorExtensionTemplate(compositor)
    , m_compositor(compositor)
{
}

void PluginScaleManager::setPluginScale(const uint32_t &scale)
{
    if (scale == m_scale)
        return;
    m_scale = scale;
    if (!m_compositor)
        return;

    auto outputs = m_compositor->outputs();
    std::for_each(outputs.begin(), outputs.end(), [this](auto *output) {
        // 120 is base of fractional scale.
        output->setScaleFactor(std::ceil(m_scale / 120.0));
    });

    Q_EMIT pluginScaleChanged(m_scale);
}

uint32_t PluginScaleManager::pluginScale()
{
    return m_scale;
}

void PluginScaleManager::initialize()
{
    QWaylandCompositorExtensionTemplate::initialize();
    QWaylandCompositor *compositor = static_cast<QWaylandCompositor *>(extensionContainer());
    Q_ASSERT(compositor);

    init(compositor->display(), 1);
    m_compositor = compositor;
    connect(compositor, &QWaylandCompositor::outputAdded, this, [this](auto *output) {
        output->setScaleFactor(std::ceil(m_scale / 120.0));
    });
}

void PluginScaleManager::wp_fractional_scale_manager_v1_get_fractional_scale(Resource *resource, uint32_t id, struct ::wl_resource *surface)
{
    QWaylandSurface *qwaylandSurface = QWaylandSurface::fromResource(surface);
    QWaylandResource shellSurfaceResource(
        wl_resource_create(resource->client(), &::wp_fractional_scale_v1_interface, wl_resource_get_version(resource->handle), id));
    auto pluginScale = new PluginScale(this, qwaylandSurface, shellSurfaceResource);
    pluginScale->send_preferred_scale(m_scale);
}

PluginScale::PluginScale(PluginScaleManager *manager, QWaylandSurface *surface, const QWaylandResource &resource)
{
    setParent(manager);
    init(resource.resource());
    setExtensionContainer(surface);
    QWaylandCompositorExtension::initialize();

    connect(manager, &PluginScaleManager::pluginScaleChanged, this, [this](uint32_t scale) {
        send_preferred_scale(scale);
    });
}

void PluginScale::wp_fractional_scale_v1_destroy(Resource *resource)
{
    Q_UNUSED(resource)
    deleteLater();
}

PluginSurface::PluginSurface(PluginManager *manager,
                             const QString &pluginId,
                             const QString &itemKey,
                             const QString &displayName,
                             int pluginFlags,
                             int pluginType,
                             int sizePolicy,
                             QWaylandSurface *surface,
                             const QWaylandResource &resource)
    : m_manager(manager)
    , m_surface(surface)
    , m_itemKey(itemKey)
    , m_pluginId(pluginId)
    , m_displayName(displayName)
    , m_flags(pluginFlags)
    , m_pluginType(pluginType)
    , m_sizePolicy(sizePolicy)
    , m_margins(0)
    , m_height(1)
    , m_width(1)
{
    init(resource.resource());
    setExtensionContainer(surface);
    QWaylandCompositorExtension::initialize();
}

QWaylandQuickShellIntegration* PluginSurface::createIntegration(QWaylandQuickShellSurfaceItem *item)
{
    return new PluginManagerIntegration(item);
}

QWaylandSurface* PluginSurface::surface() const
{
    return m_surface;
}

QString PluginSurface::pluginId() const
{
    return m_pluginId;
}

QString PluginSurface::itemKey() const
{
    return m_itemKey;
}

QString PluginSurface::displayName() const
{
    return m_displayName;
}

uint32_t PluginSurface::pluginType() const
{
    return m_pluginType;
}

uint32_t PluginSurface::pluginFlags() const
{
    return m_flags;
}

uint32_t PluginSurface::pluginSizePolicy () const
{
    return m_sizePolicy;
}

int PluginSurface::height() const
{
    return m_height;
}

int PluginSurface::width() const
{
    return m_width;
}

QString PluginSurface::dccIcon() const
{
    return m_dccIcon;
}

void PluginSurface::setItemActive(bool isActive)
{
    if (m_isItemActive == isActive) {
        return;
    }

    m_isItemActive = isActive;
    Q_EMIT itemActiveChanged();
}

bool PluginSurface::isItemActive() const
{
    return m_isItemActive;
}

void PluginSurface::updatePluginGeometry(const QRect &geometry)
{
    send_geometry(geometry.x(), geometry.y(), geometry.width(), geometry.height());
}

void PluginSurface::plugin_mouse_event(QtWaylandServer::plugin::Resource *resource, int32_t type)
{
    Q_UNUSED(resource)
    qInfo() << "server plugin surface receive mouse event:" << type;
    Q_EMIT recvMouseEvent((QEvent::Type)type);
}

void PluginSurface::plugin_dcc_icon(Resource *resource, const QString &icon)
{
    Q_UNUSED(resource)
    qInfo() << "dcc_icon:" << icon;
    m_dccIcon = icon;
}

void PluginSurface::plugin_request_shutdown(Resource *resource, const QString &type)
{
    Q_UNUSED(resource);
    Q_EMIT m_manager->requestShutdown(type);
}

void PluginSurface::plugin_close_quick_panel(Resource *resource) {
    Q_UNUSED(resource)
    qInfo() << "close_quick_panel";
    Q_EMIT m_manager->pluginCloseQuickPanelPopup();
}

void PluginSurface::plugin_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    Q_EMIT aboutToDestroy();
    m_manager->removePluginSurface(this);
    delete this;
}

void PluginSurface::plugin_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void PluginSurface::setGlobalPos(const QPoint &pos)
{
    QRect g = qApp->primaryScreen() ? qApp->primaryScreen()->geometry() : QRect();
    for (auto *screen : qApp->screens())
    {
        const QRect &sg = screen->geometry();
        if (sg.contains(pos))
        {
            g = sg;
            break;
        }
    }

    auto p = g.topLeft() + (pos - g.topLeft()) * qApp->devicePixelRatio();
    send_raw_global_pos(p.x(), p.y());
}

void PluginSurface::plugin_source_size(Resource *resource, int32_t width, int32_t height)
{
    Q_UNUSED(resource);
    if (width == 0 || height == 0)
        return;

    if (height != m_height) {
        m_height = height;
        Q_EMIT heightChanged();
    }

    if (width != m_width) {
        m_width = width;
        Q_EMIT widthChanged();
    }
}

PluginPopup::PluginPopup(PluginManager *manager,
                         const QString &pluginId,
                         const QString &itemKey,
                         int x,
                         int y,
                         int popupType,
                         QWaylandSurface *surface,
                         const QWaylandResource &resource)
    : m_manager(manager)
    , m_surface(surface)
    , m_itemKey(itemKey)
    , m_pluginId(pluginId)
    , m_popupType(popupType)
    , m_height(1)
    , m_width(1)
{
    Q_UNUSED(x)
    Q_UNUSED(y)
    init(resource.resource());
    setExtensionContainer(surface);
    QWaylandCompositorExtension::initialize();
}

QWaylandSurface* PluginPopup::surface() const
{
    return m_surface;
}

QWaylandQuickShellIntegration* PluginPopup::createIntegration(QWaylandQuickShellSurfaceItem *item)
{
    return new PluginPopupIntegration(item);
}

void PluginPopup::updatePluginGeometry(const QRect &geometry)
{
    send_geometry(geometry.x(), geometry.y(), geometry.width(), geometry.height());
}

void PluginPopup::setEmbedPanelMinHeight(int height)
{
    m_manager->setEmbedPanelMinHeight(height);
}

QString PluginPopup::pluginId() const
{
    return m_pluginId;
}

QString PluginPopup::itemKey() const
{
    return m_itemKey;
}

int32_t PluginPopup::x() const
{
    return m_x;
}

int32_t PluginPopup::y() const
{
    return m_y;
}

void PluginPopup::setX(int32_t x)
{
    m_x = x;
    Q_EMIT xChanged();
}

void PluginPopup::setY(int32_t y)
{
    m_y = y;
    Q_EMIT yChanged();
}

int PluginPopup::height() const
{
    return m_height;
}

int PluginPopup::width() const
{
    return m_width;
}

int32_t PluginPopup::popupType() const
{
    return m_popupType;
}

void PluginPopup::plugin_popup_set_position(Resource *resource, int32_t x, int32_t y)
{
    Q_UNUSED(resource)
    setX(x);
    setY(y);
}

void PluginPopup::plugin_popup_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    // TODO why we get a same address with the object when new PluginPopup, if we delete the object.
    Q_EMIT aboutToDestroy();
    delete this;
}

void PluginPopup::plugin_popup_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void PluginPopup::plugin_popup_source_size(Resource *resource, int32_t width, int32_t height)
{
    Q_UNUSED(resource);
    if (width == 0 || height == 0)
        return;

    if (height != m_height) {
        m_height = height;
        Q_EMIT heightChanged();
    }

    if (width != m_width) {
        m_width = width;
        Q_EMIT widthChanged();
    }
}

void PluginPopup::plugin_popup_set_cursor(Resource *resource, int32_t cursor_shape)
{
    Q_UNUSED(resource);
    Q_EMIT cursorShapeRequested(cursor_shape);
}

PluginManager::PluginManager(QWaylandCompositor *compositor)
    : QWaylandCompositorExtensionTemplate(compositor)
{
    auto theme = DGuiApplicationHelper::instance()->applicationTheme();
    QObject::connect(theme, &DPlatformTheme::fontNameChanged, this, &PluginManager::onFontChanged);
    QObject::connect(theme, &DPlatformTheme::fontPointSizeChanged, this, &PluginManager::onFontChanged);
    QObject::connect(theme, &DPlatformTheme::activeColorChanged, this, &PluginManager::onActiveColorChanged);
    QObject::connect(theme, &DPlatformTheme::darkActiveColorChanged, this, &PluginManager::onActiveColorChanged);
    QObject::connect(theme, &DPlatformTheme::themeNameChanged, this, &PluginManager::onThemeChanged);
    QObject::connect(theme, &DPlatformTheme::iconThemeNameChanged, this, &PluginManager::onThemeChanged);
}

void PluginManager::initialize()
{
    QWaylandCompositorExtensionTemplate::initialize();
    QWaylandCompositor *compositor = static_cast<QWaylandCompositor *>(extensionContainer());

    // ###(zccrs): 在dde-shell中不要使用QWaylandCompositor的event handler，它会对key event进行
    // 特殊处理，会丢弃掉原生事件的信息，仅根据 native scan key code 通过xkb生成原始key event向
    // dde-shell 传递，这会导致需要状态切换后进行输入的字符丢失信息，比如外部环境打开NumLock后，小键盘
    // 的数字输入会被event handler转成原始事件，丢失了NumLock的开关信息。
    // dde-shell不是一个独立的合成器，所以不需要额外处理key event，需要遵守原始事件中的NumLock状态
    // 确保输出数字时在dde-shell的wayland客户端中接收到的也是数字。
    auto eventHandler = QWaylandCompositorPrivate::get(compositor)->eventHandler.get();
    if (eventHandler == QWindowSystemInterfacePrivate::eventHandler) {
        QWindowSystemInterfacePrivate::removeWindowSystemEventhandler(eventHandler);
    }

    // TODO: Use built-in protocol instead of qt
#ifdef USE_DEEPIN_QT
    // 只创建就行
    auto qtKey = new QtWayland::QtKeyExtensionGlobal(compositor);
    qtKey->setParent(compositor);
    auto qtTouch = new QtWayland::TouchExtensionGlobal(compositor);
    qtTouch->setParent(compositor);
#endif

    init(compositor->display(), 1);

    // We ONLY register QtTextInputMethodManager.
    // If we register v2 or v3, they will intercept `inputMethodEvent` inside `QWaylandInputMethodControl::inputMethodEvent`
    // because Qt's server-side logic checks v2 -> v3 -> QtTextInputMethod.
    // But the Qt 6 client-side creates QWaylandInputMethodContext which only listens to QtTextInputMethod
    // if it is available. This causes mismatched preferences: server sends via v2/v3, client listens to QtTextInputMethod.
    // So ONLY register QtTextInputMethodManager.
    auto qtTextInputMethodManager = new QWaylandQtTextInputMethodManager(compositor);
    qtTextInputMethodManager->initialize();

    // 设置鼠标焦点监听器
    setupMouseFocusListener();

    // 设置 text input 焦点代理
    setupTextInputProxy(compositor);

    // 启用剪贴板保留并在宿主系统剪贴板更新时同步给插件 Wayland 客户端，实现粘贴功能
    compositor->setRetainedSelectionEnabled(true);
    if (auto *clipboard = QGuiApplication::clipboard()) {
        auto syncClipboardData = [compositor](const QMimeData *mimeData) {
            if (!mimeData) return;
            QMimeData lightweightData;
            static QStringList allowedFormats = {"text/plain", "text/html"};
            bool hasData = false;
            
            for (const QString &format : mimeData->formats()) {
                if (allowedFormats.contains(format)) {
                    lightweightData.setData(format, mimeData->data(format));
                    hasData = true;
                }
            }

            if (hasData) {
                compositor->overrideSelection(&lightweightData);
            }
        };

        QObject::connect(clipboard, &QClipboard::changed, this, [clipboard, syncClipboardData](QClipboard::Mode mode) {
            if (mode == QClipboard::Clipboard) {
                syncClipboardData(clipboard->mimeData(mode));
            }
        });

        syncClipboardData(clipboard->mimeData(QClipboard::Clipboard));
    }
}

void PluginManager::updateDockOverflowState(int state)
{
    QJsonObject obj;
    obj[dock::MSG_TYPE] = dock::MSG_UPDATE_OVERFLOW_STATE;
    obj[dock::MSG_DATA] = state;

    sendEventMsg(toJson(obj));
}

void PluginManager::setPopupMinHeight(int height)
{
    setEmbedPanelMinHeight(height);
}

uint32_t PluginManager::dockPosition() const
{
    return m_dockPosition;
}

void PluginManager::setDockPosition(uint32_t dockPosition)
{
    if (m_dockPosition == dockPosition)
        return;

    m_dockPosition = dockPosition;
    foreach (PluginSurface *plugin, m_pluginSurfaces) {
        Resource *target = resourceMap().value(plugin->surface()->waylandClient());
        if (target) {
            send_position_changed(target->handle, m_dockPosition);
        }
    }
}

uint32_t PluginManager::dockColorTheme() const
{
    return m_dockColorTheme;
}

void PluginManager::setDockColorTheme(uint32_t type)
{
    if (type == m_dockColorTheme)
        return;

    m_dockColorTheme = type;
    foreach (PluginSurface *plugin, m_pluginSurfaces) {
        Resource *target = resourceMap().value(plugin->surface()->waylandClient());
        if (target) {
            send_color_theme_changed(target->handle, m_dockColorTheme);
        }
    }
}

void PluginManager::setEmbedPanelMinHeight(int height)
{
    if (m_popupMinHeight == height)
        return;
    m_popupMinHeight = height;

    sendEventMsg(popupMinHeightMsg());
}

void PluginManager::plugin_manager_v1_request_message(Resource *resource, const QString &plugin_id, const QString &item_key, const QString &msg)
{
    Q_UNUSED(resource)
    qInfo() << "server pluginManager receive client:" << plugin_id << item_key << " msg:" << msg;
    PluginSurface *dstPlugin = nullptr;
    for (PluginSurface *plugin : m_pluginSurfaces) {
        if (plugin->pluginId() == plugin_id && plugin->itemKey() == item_key) {
            dstPlugin = plugin;
            break;
        }
    }

    if (!dstPlugin) {
        return;
    }

    auto rootObj = getRootObj(msg);
    if (rootObj.isEmpty()) {
        return;
    }

    const auto &msgType = rootObj.value(dock::MSG_TYPE);
    if (msgType == dock::MSG_SUPPORT_FLAG_CHANGED) {
        // todo
    } else if (msgType == dock::MSG_ITEM_ACTIVE_STATE) {
        dstPlugin->setItemActive(rootObj.value(dock::MSG_DATA).toBool());
    } else if (msgType == dock::MSG_UPDATE_TOOLTIPS_VISIBLE) {

    }
}

void PluginManager::plugin_manager_v1_create_plugin(Resource *resource, const QString &pluginId, const QString &itemKey, const QString &display_name, int32_t plugin_flags, int32_t type, int32_t size_policy, struct ::wl_resource *surface, uint32_t id)
{
    QWaylandSurface *qwaylandSurface = QWaylandSurface::fromResource(surface);

    QWaylandResource shellSurfaceResource(wl_resource_create(resource->client(), &::plugin_interface,
                                                           wl_resource_get_version(resource->handle), id));

    send_position_changed(resource->handle, m_dockPosition);
    send_color_theme_changed(resource->handle, m_dockColorTheme);
    auto theme = DGuiApplicationHelper::instance()->applicationTheme();
    send_active_color_changed(resource->handle, theme->activeColor().name(), theme->darkActiveColor().name());
    send_font_changed(resource->handle, theme->fontName(), theme->fontPointSize());
    send_theme_changed(resource->handle, theme->themeName(), theme->iconThemeName());

    auto plugin = new PluginSurface(this, pluginId, itemKey, display_name, plugin_flags, type, size_policy, qwaylandSurface, shellSurfaceResource);
    m_pluginSurfaces << plugin;
    Q_EMIT pluginSurfaceCreated(plugin);

    sendEventMsg(resource, dockSizeMsg());
    sendEventMsg(resource, popupMinHeightMsg());
}

void PluginManager::plugin_manager_v1_create_popup_at(Resource *resource, const QString &pluginId, const QString &itemKey, int32_t type, int32_t x, int32_t y, struct ::wl_resource *surface, uint32_t id)
{
    QWaylandSurface *qwaylandSurface = QWaylandSurface::fromResource(surface);
    QWaylandResource shellSurfaceResource(wl_resource_create(resource->client(), &::plugin_popup_interface,
                                                           wl_resource_get_version(resource->handle), id));

    auto plugin = new PluginPopup(this, pluginId, itemKey, x, y, type, qwaylandSurface, shellSurfaceResource);
    plugin->setX(x), plugin->setY(y);
    Q_EMIT pluginPopupCreated(plugin);
}

QJsonObject PluginManager::getRootObj(const QString &jsonStr) {
    QJsonParseError jsonParseError;
    const QJsonDocument &resultDoc = QJsonDocument::fromJson(jsonStr.toLocal8Bit(), &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError || resultDoc.isEmpty()) {
        qWarning() << "Result json parse error";
        return QJsonObject();
    }

    return resultDoc.object();
}

QString PluginManager::toJson(const QJsonObject &jsonObj)
{
    QJsonDocument doc;
    doc.setObject(jsonObj);
    return doc.toJson();
}

void PluginManager::sendEventMsg(const QString &msg)
{
    foreach (PluginSurface *plugin, m_pluginSurfaces) {
        Resource *target = resourceMap().value(plugin->surface()->waylandClient());
        sendEventMsg(target, msg);
    }
}

void PluginManager::sendEventMsg(Resource *target, const QString &msg)
{
    if (target && !msg.isEmpty()) {
        send_event_message(target->handle, msg);
    }
}

int PluginSurface::margins() const
{
    return m_margins;
}

void PluginSurface::setMargins(int newMargins)
{
    if (m_margins == newMargins)
        return;
    m_margins = newMargins;
    send_margin(m_margins);

    emit marginsChanged();
}

QSize PluginManager::dockSize() const
{
    return m_dockSize;
}

void PluginManager::setDockSize(const QSize &newDockSize)
{
    if (m_dockSize == newDockSize)
        return;
    m_dockSize = newDockSize;
    sendEventMsg(dockSizeMsg());
    emit dockSizeChanged();
}

void PluginManager::removePluginSurface(PluginSurface *plugin)
{
    Q_EMIT pluginSurfaceDestroyed(plugin);
    m_pluginSurfaces.removeAll(plugin);
}

void PluginManager::onFontChanged()
{
    foreachPluginSurface([this](Resource *source) {
        auto theme = DGuiApplicationHelper::instance()->applicationTheme();
        send_font_changed(source->handle, theme->fontName(), theme->fontPointSize());
    });
}

void PluginManager::onActiveColorChanged()
{
    foreachPluginSurface([this](Resource *source) {
        auto theme = DGuiApplicationHelper::instance()->applicationTheme();
        send_active_color_changed(source->handle, theme->activeColor().name(), theme->darkActiveColor().name());
    });
}

void PluginManager::onThemeChanged()
{
    foreachPluginSurface([this](Resource *source) {
        auto theme = DGuiApplicationHelper::instance()->applicationTheme();
        send_theme_changed(source->handle, theme->themeName(), theme->iconThemeName());
    });
}

void PluginManager::foreachPluginSurface(PluginSurfaceCallback callback)
{
    foreach (PluginSurface *plugin, m_pluginSurfaces) {
        Resource *target = resourceMap().value(plugin->surface()->waylandClient());
        if (target) {
            callback(target);
        }
    }
}

QString PluginManager::dockSizeMsg() const
{
    if (m_dockSize.isEmpty())
        return QString();

    QJsonObject sizeData;
    sizeData["width"] = m_dockSize.width();
    sizeData["height"] = m_dockSize.height();

    QJsonObject obj;
    obj[dock::MSG_TYPE] = dock::MSG_DOCK_PANEL_SIZE_CHANGED;
    obj[dock::MSG_DATA] = sizeData;
    return toJson(obj);
}

QString PluginManager::popupMinHeightMsg() const
{
    if (m_popupMinHeight <= 0)
        return QString();

    QJsonObject obj;
    obj[dock::MSG_TYPE] = dock::MSG_SET_APPLET_MIN_HEIGHT;
    obj[dock::MSG_DATA] = m_popupMinHeight;

    return toJson(obj);
}

void PluginManager::setupMouseFocusListener()
{
    QWaylandCompositor *compositor = static_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;

    QWaylandSeat *seat = compositor->defaultSeat();
    if (!seat)
        return;

    QObject::connect(seat, &QWaylandSeat::mouseFocusChanged, this,
        [seat](QWaylandView *newFocus, QWaylandView *oldFocus) {
            Q_UNUSED(oldFocus);
            if(!newFocus)
                return;

            if (auto surface = newFocus->surface()) {
                seat->setKeyboardFocus(surface);
            }
        });
}

void PluginManager::setupTextInputProxy(QWaylandCompositor *compositor)
{
    // === IME 事件转发的完整链路说明 ===
    //
    // 【步骤1】设置 text input 协议对象的 focus（keyboardFocusChanged 监听器）
    // 当插件 surface 的键盘焦点发生变化时，需要同步更新所有 text input 协议对象的焦点。
    // QWaylandSeat::setKeyboardFocus() 只处理 wl_keyboard 协议的 enter/leave，
    // 不会自动通知 text input 协议对象（QWaylandTextInput/V3/QtTextInputMethod）。
    // 因此需要在 keyboardFocusChanged 信号中手动调用各协议对象的 setFocus()。
    // 这样，当插件进程的输入框触发 text_input::enable 时，compositor 端的 text input
    // 对象已有正确的 focus surface，surfaceEnabled 信号会正确触发，进而：
    //   → QWaylandInputMethodControl::setEnabled(true)
    //   → QWaylandQuickItem::updateInputMethod() 设上 ItemAcceptsInputMethod flag
    //
    // 【步骤2】让 QWaylandQuickItem 获取 Qt active focus（surfaceEnabled 监听器）
    // Qt 的 QInputMethodEvent 只会发给当前有 active focus 的 QQuickItem。
    // 即便 ShellSurfaceItem/QWaylandQuickItem 已设上 ItemAcceptsInputMethod flag，
    // 如果它没有 Qt active focus，外层 compositor 发来的 commit_string 生成的
    // QInputMethodEvent 就会丢失，中文字无法提交到插件进程的输入框。
    // 因此，当 surfaceEnabled 触发时（插件进程输入框 enable IME），需要通过
    // surface->views() 找到对应的 QWaylandQuickItem，调用 forceActiveFocus()，
    // 确保 QInputMethodEvent 能正确路由到
    //   QWaylandQuickItem::inputMethodEvent()
    //   → QWaylandInputMethodControl::inputMethodEvent()
    //   → QWaylandTextInput::sendInputMethodEvent()
    //   → send_commit_string() 到插件进程
    //
    // 注意：QWaylandTextInput 等 per-seat 对象是在客户端绑定全局时懒创建的，
    // 初始化时 seat->extensions() 为空。因此步骤2的连接放在
    // keyboardFocusChanged lambda 内部（那时扩展已经存在）。

    QWaylandSeat *seat = compositor->defaultSeat();
    if (!seat)
        return;

    // 【步骤1 & 2 实现】在 keyboardFocusChanged 中同时完成两件事：
    //   a) 同步 text input 协议对象的焦点（步骤1）
    //   b) 用 Qt::UniqueConnection 连接 surfaceEnabled，让 QWaylandQuickItem 获取 active focus（步骤2）
    QObject::connect(seat, &QWaylandSeat::keyboardFocusChanged, this, [this, seat]
        (QWaylandSurface *newFocus, QWaylandSurface *oldFocus) {
        Q_UNUSED(oldFocus);

        if (newFocus) {
            newFocus->updateSelection();
        }

        // 由于 Deepin Qt6 构建中 QWaylandTextInput/V3 的公开类符号没有导出，
        // 无法使用 findIn() 和公开的 setFocus()。
        // 改用 Private API：遍历 seat 的 extension 列表，
        // 通过 QObject 元对象系统找到 text input 对象，
        // 再通过 d_func() 调用 Private 类的 setFocus()。
        const auto extensions = seat->extensions();
        for (auto *ext : extensions) {
            const QString className = ext->metaObject()->className();
            if (className == QStringLiteral("QWaylandTextInput")) {
                // 步骤1：设置 text input 焦点
                auto *d = static_cast<QWaylandTextInputPrivate *>(QObjectPrivate::get(ext));
                d->setFocus(newFocus);
                // 步骤2：连接 surfaceEnabled，当插件输入框 enable IME 时，
                // 让对应的 QWaylandQuickItem 获取 Qt active focus。
                // Qt::UniqueConnection 防止重复连接。
                QObject::connect(ext, SIGNAL(surfaceEnabled(QWaylandSurface*)),
                    this, SLOT(onTextInputSurfaceEnabled(QWaylandSurface*)),
                    Qt::UniqueConnection);
            } else if (className == QStringLiteral("QWaylandTextInputV3")) {
                auto *d = static_cast<QWaylandTextInputV3Private *>(QObjectPrivate::get(ext));
                d->setFocus(newFocus);
                // 同样连接 v3 的 surfaceEnabled（fcitx5/ibus 等可能使用 v3）
                QObject::connect(ext, SIGNAL(surfaceEnabled(QWaylandSurface*)),
                    this, SLOT(onTextInputSurfaceEnabled(QWaylandSurface*)),
                    Qt::UniqueConnection);
            } else if (className == QStringLiteral("QWaylandQtTextInputMethod")) {
                auto *qtMethodPriv = static_cast<QWaylandQtTextInputMethodPrivate *>(QObjectPrivate::get(ext));
                qtMethodPriv->inputPanelVisible = true; // ensure IME panel can be shown
                WlQtTextInputMethodHelper::setFocusCustom(qtMethodPriv, newFocus);
                // Qt6 WPA 默认优先使用此私有协议（qt_text_input_method_manager_v1），
                // 插件进程通常通过此协议发 enable，必须连接其 surfaceEnabled。
                QObject::connect(ext, SIGNAL(surfaceEnabled(QWaylandSurface*)),
                    this, SLOT(onTextInputSurfaceEnabled(QWaylandSurface*)),
                    Qt::UniqueConnection);
            }
        }
    });
}

void PluginManager::onTextInputSurfaceEnabled(QWaylandSurface *surface)
{
    if (!surface)
        return;

    // 通过 surface 的 views 找到对应的 QWaylandQuickItem，
    // 调用 forceActiveFocus() 使其成为 Qt 的 active focus item，
    // 这样外层 compositor 的 QInputMethodEvent 才能被路由到它。
    const auto views = surface->views();
    for (auto *view : views) {
        if (auto *quickItem = qobject_cast<QWaylandQuickItem *>(view->renderObject())) {
            quickItem->forceActiveFocus(Qt::OtherFocusReason);
            break;
        }
    }
}
