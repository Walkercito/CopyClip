#include "ui/StatusNotifierItem.hpp"

#include "config/Constants.hpp"

#include <gio/gio.h>

#include <spdlog/spdlog.h>

#include <array>
#include <string>
#include <utility>

#include <unistd.h>

namespace copyclip::ui {

namespace {

// The StatusNotifierWatcher we register with (owned by the host: GNOME's
// AppIndicator extension, KDE, XFCE, etc.).
constexpr const char* kWatcherName = "org.kde.StatusNotifierWatcher";
constexpr const char* kWatcherPath = "/StatusNotifierWatcher";
constexpr const char* kWatcherIface = "org.kde.StatusNotifierWatcher";

constexpr const char* kItemIface = "org.kde.StatusNotifierItem";
constexpr const char* kItemPath = "/StatusNotifierItem";
constexpr const char* kMenuIface = "com.canonical.dbusmenu";
constexpr const char* kMenuPath = "/MenuBar";

// Menu item ids (0 is the invisible root).
constexpr int kIdOpen = 1;
constexpr int kIdSettings = 2;
constexpr int kIdQuit = 3;

struct MenuEntry {
    int id;
    const char* label;
};
constexpr std::array<MenuEntry, 3> kMenuEntries{{
    {kIdOpen, "Show Clipboard"},
    {kIdSettings, "Settings"},
    {kIdQuit, "Quit"},
}};

constexpr const char* kItemXml = R"XML(
<node>
  <interface name="org.kde.StatusNotifierItem">
    <property name="Category" type="s" access="read"/>
    <property name="Id" type="s" access="read"/>
    <property name="Title" type="s" access="read"/>
    <property name="Status" type="s" access="read"/>
    <property name="IconName" type="s" access="read"/>
    <property name="IconThemePath" type="s" access="read"/>
    <property name="ItemIsMenu" type="b" access="read"/>
    <property name="Menu" type="o" access="read"/>
    <method name="Activate">
      <arg name="x" type="i" direction="in"/>
      <arg name="y" type="i" direction="in"/>
    </method>
    <method name="SecondaryActivate">
      <arg name="x" type="i" direction="in"/>
      <arg name="y" type="i" direction="in"/>
    </method>
    <method name="ContextMenu">
      <arg name="x" type="i" direction="in"/>
      <arg name="y" type="i" direction="in"/>
    </method>
    <method name="Scroll">
      <arg name="delta" type="i" direction="in"/>
      <arg name="orientation" type="s" direction="in"/>
    </method>
    <signal name="NewIcon"/>
    <signal name="NewTitle"/>
    <signal name="NewStatus">
      <arg name="status" type="s"/>
    </signal>
  </interface>
</node>)XML";

constexpr const char* kMenuXml = R"XML(
<node>
  <interface name="com.canonical.dbusmenu">
    <property name="Version" type="u" access="read"/>
    <property name="Status" type="s" access="read"/>
    <property name="TextDirection" type="s" access="read"/>
    <property name="IconThemePath" type="as" access="read"/>
    <method name="GetLayout">
      <arg name="parentId" type="i" direction="in"/>
      <arg name="recursionDepth" type="i" direction="in"/>
      <arg name="propertyNames" type="as" direction="in"/>
      <arg name="revision" type="u" direction="out"/>
      <arg name="layout" type="(ia{sv}av)" direction="out"/>
    </method>
    <method name="GetGroupProperties">
      <arg name="ids" type="ai" direction="in"/>
      <arg name="propertyNames" type="as" direction="in"/>
      <arg name="properties" type="a(ia{sv})" direction="out"/>
    </method>
    <method name="GetProperty">
      <arg name="id" type="i" direction="in"/>
      <arg name="name" type="s" direction="in"/>
      <arg name="value" type="v" direction="out"/>
    </method>
    <method name="Event">
      <arg name="id" type="i" direction="in"/>
      <arg name="eventId" type="s" direction="in"/>
      <arg name="data" type="v" direction="in"/>
      <arg name="timestamp" type="u" direction="in"/>
    </method>
    <method name="EventGroup">
      <arg name="events" type="a(isvu)" direction="in"/>
      <arg name="idErrors" type="ai" direction="out"/>
    </method>
    <method name="AboutToShow">
      <arg name="id" type="i" direction="in"/>
      <arg name="needUpdate" type="b" direction="out"/>
    </method>
    <method name="AboutToShowGroup">
      <arg name="ids" type="ai" direction="in"/>
      <arg name="updatesNeeded" type="ai" direction="out"/>
      <arg name="idErrors" type="ai" direction="out"/>
    </method>
    <signal name="ItemsPropertiesUpdated">
      <arg name="updatedProps" type="a(ia{sv})"/>
      <arg name="removedProps" type="a(ias)"/>
    </signal>
    <signal name="LayoutUpdated">
      <arg name="revision" type="u"/>
      <arg name="parent" type="i"/>
    </signal>
  </interface>
</node>)XML";

// One "{sv}" dict entry: a string key mapping to a variant-wrapped value.
GVariant* dict_entry(const char* key, GVariant* value) {
    return g_variant_new_dict_entry(g_variant_new_string(key), g_variant_new_variant(value));
}

// The dbusmenu "a{sv}" property map for a plain, enabled, visible menu item.
GVariant* item_props(const char* label) {
    std::array<GVariant*, 3> entries{
        dict_entry("label", g_variant_new_string(label)),
        dict_entry("enabled", g_variant_new_boolean(TRUE)),
        dict_entry("visible", g_variant_new_boolean(TRUE)),
    };
    return g_variant_new_array(G_VARIANT_TYPE("{sv}"), entries.data(), entries.size());
}

// One "(ia{sv}av)" node for a leaf menu item (empty children array).
GVariant* build_menu_item(const MenuEntry& entry) {
    std::array<GVariant*, 3> fields{
        g_variant_new_int32(entry.id),
        item_props(entry.label),
        g_variant_new_array(G_VARIANT_TYPE("v"), nullptr, 0),
    };
    return g_variant_new_tuple(fields.data(), fields.size());
}

// The full "(ia{sv}av)" layout: an invisible root (id 0) holding the menu items.
GVariant* build_layout() {
    std::array<GVariant*, kMenuEntries.size()> children{};
    for (std::size_t i = 0; i < kMenuEntries.size(); ++i) {
        children.at(i) = g_variant_new_variant(build_menu_item(kMenuEntries.at(i)));
    }
    GVariant* root_child_array =
        g_variant_new_array(G_VARIANT_TYPE("v"), children.data(), children.size());

    GVariant* root_prop = dict_entry("children-display", g_variant_new_string("submenu"));
    GVariant* root_props = g_variant_new_array(G_VARIANT_TYPE("{sv}"), &root_prop, 1);

    std::array<GVariant*, 3> fields{g_variant_new_int32(0), root_props, root_child_array};
    return g_variant_new_tuple(fields.data(), fields.size());
}

} // namespace

StatusNotifierItem::StatusNotifierItem(std::string icon_name, Callback on_open,
                                       Callback on_settings, Callback on_quit)
    : icon_name_{std::move(icon_name)}, on_open_{std::move(on_open)},
      on_settings_{std::move(on_settings)}, on_quit_{std::move(on_quit)},
      bus_name_{std::string{kItemIface} + "-" + std::to_string(getpid()) + "-1"},
      item_node_{g_dbus_node_info_new_for_xml(kItemXml, nullptr)},
      menu_node_{g_dbus_node_info_new_for_xml(kMenuXml, nullptr)} {
    if (item_node_ == nullptr || menu_node_ == nullptr) {
        spdlog::warn("panel icon: could not parse D-Bus interfaces; icon disabled");
        return;
    }
    owner_id_ =
        g_bus_own_name(G_BUS_TYPE_SESSION, bus_name_.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE,
                       &StatusNotifierItem::on_bus_acquired, nullptr, nullptr, this, nullptr);
}

StatusNotifierItem::~StatusNotifierItem() {
    if (connection_ != nullptr && item_registration_ != 0) {
        g_dbus_connection_unregister_object(connection_, item_registration_);
    }
    if (connection_ != nullptr && menu_registration_ != 0) {
        g_dbus_connection_unregister_object(connection_, menu_registration_);
    }
    if (owner_id_ != 0) {
        g_bus_unown_name(owner_id_);
    }
    if (item_node_ != nullptr) {
        g_dbus_node_info_unref(item_node_);
    }
    if (menu_node_ != nullptr) {
        g_dbus_node_info_unref(menu_node_);
    }
}

void StatusNotifierItem::on_bus_acquired(GDBusConnection* connection, const char* /*name*/,
                                         gpointer self_ptr) {
    auto* self = static_cast<StatusNotifierItem*>(self_ptr);
    self->connection_ = connection;

    static const GDBusInterfaceVTable item_vtable{
        &StatusNotifierItem::handle_item_call, &StatusNotifierItem::handle_item_get, nullptr, {}};
    static const GDBusInterfaceVTable menu_vtable{
        &StatusNotifierItem::handle_menu_call, &StatusNotifierItem::handle_menu_get, nullptr, {}};

    self->item_registration_ = g_dbus_connection_register_object(
        connection, kItemPath, g_dbus_node_info_lookup_interface(self->item_node_, kItemIface),
        &item_vtable, self, nullptr, nullptr);
    self->menu_registration_ = g_dbus_connection_register_object(
        connection, kMenuPath, g_dbus_node_info_lookup_interface(self->menu_node_, kMenuIface),
        &menu_vtable, self, nullptr, nullptr);
    if (self->item_registration_ == 0 || self->menu_registration_ == 0) {
        spdlog::warn("panel icon: could not export D-Bus objects; icon disabled");
        return;
    }

    // Register with the host's watcher. If none is present (e.g. GNOME without the
    // AppIndicator extension) the call errors and the icon simply never appears —
    // logged at debug, never fatal.
    GVariant* arg = g_variant_new_string(self->bus_name_.c_str());
    g_dbus_connection_call(
        connection, kWatcherName, kWatcherPath, kWatcherIface, "RegisterStatusNotifierItem",
        g_variant_new_tuple(&arg, 1), nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
        +[](GObject* source, GAsyncResult* result, gpointer /*data*/) {
            GError* error = nullptr;
            GVariant* reply =
                g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), result, &error);
            if (error != nullptr) {
                spdlog::debug("panel icon: no StatusNotifierWatcher ({}); icon not shown",
                              error->message);
                g_error_free(error);
                return;
            }
            g_variant_unref(reply);
        },
        nullptr);
}

void StatusNotifierItem::handle_item_call(GDBusConnection* /*connection*/, const char* /*sender*/,
                                          const char* /*path*/, const char* /*iface*/,
                                          const char* method, GVariant* /*params*/,
                                          GDBusMethodInvocation* invocation, gpointer self_ptr) {
    auto* self = static_cast<StatusNotifierItem*>(self_ptr);
    // Left-click "Activate" is only delivered by some hosts; treat it as Open. The
    // reliable path on GNOME is the menu (handled in handle_menu_call).
    if (g_strcmp0(method, "Activate") == 0 && self->on_open_) {
        self->on_open_();
    }
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

GVariant* StatusNotifierItem::handle_item_get(GDBusConnection* /*connection*/,
                                              const char* /*sender*/, const char* /*path*/,
                                              const char* /*iface*/, const char* property,
                                              GError** /*error*/, gpointer self_ptr) {
    auto* self = static_cast<StatusNotifierItem*>(self_ptr);
    if (g_strcmp0(property, "Category") == 0) {
        return g_variant_new_string("ApplicationStatus");
    }
    if (g_strcmp0(property, "Id") == 0) {
        return g_variant_new_string(std::string{config::kAppId}.c_str());
    }
    if (g_strcmp0(property, "Title") == 0) {
        return g_variant_new_string(std::string{config::kAppName}.c_str());
    }
    if (g_strcmp0(property, "Status") == 0) {
        return g_variant_new_string("Active");
    }
    if (g_strcmp0(property, "IconName") == 0) {
        return g_variant_new_string(self->icon_name_.c_str());
    }
    if (g_strcmp0(property, "IconThemePath") == 0) {
        return g_variant_new_string("");
    }
    if (g_strcmp0(property, "ItemIsMenu") == 0) {
        return g_variant_new_boolean(TRUE);
    }
    if (g_strcmp0(property, "Menu") == 0) {
        return g_variant_new_object_path(kMenuPath);
    }
    return nullptr;
}

void StatusNotifierItem::dispatch_menu_event(int id) {
    if (id == kIdOpen && on_open_) {
        on_open_();
    } else if (id == kIdSettings && on_settings_) {
        on_settings_();
    } else if (id == kIdQuit && on_quit_) {
        on_quit_();
    }
}

void StatusNotifierItem::handle_menu_call(GDBusConnection* /*connection*/, const char* /*sender*/,
                                          const char* /*path*/, const char* /*iface*/,
                                          const char* method, GVariant* params,
                                          GDBusMethodInvocation* invocation, gpointer self_ptr) {
    auto* self = static_cast<StatusNotifierItem*>(self_ptr);

    if (g_strcmp0(method, "GetLayout") == 0) {
        std::array<GVariant*, 2> out{g_variant_new_uint32(1), build_layout()};
        g_dbus_method_invocation_return_value(invocation,
                                              g_variant_new_tuple(out.data(), out.size()));
        return;
    }
    if (g_strcmp0(method, "GetGroupProperties") == 0) {
        std::array<GVariant*, kMenuEntries.size()> rows{};
        for (std::size_t i = 0; i < kMenuEntries.size(); ++i) {
            std::array<GVariant*, 2> fields{g_variant_new_int32(kMenuEntries.at(i).id),
                                            item_props(kMenuEntries.at(i).label)};
            rows.at(i) = g_variant_new_tuple(fields.data(), fields.size());
        }
        GVariant* array = g_variant_new_array(G_VARIANT_TYPE("(ia{sv})"), rows.data(), rows.size());
        g_dbus_method_invocation_return_value(invocation, g_variant_new_tuple(&array, 1));
        return;
    }
    if (g_strcmp0(method, "Event") == 0) {
        GVariant* id_value = g_variant_get_child_value(params, 0);
        GVariant* event_value = g_variant_get_child_value(params, 1);
        if (g_strcmp0(g_variant_get_string(event_value, nullptr), "clicked") == 0) {
            self->dispatch_menu_event(g_variant_get_int32(id_value));
        }
        g_variant_unref(id_value);
        g_variant_unref(event_value);
        g_dbus_method_invocation_return_value(invocation, nullptr);
        return;
    }
    if (g_strcmp0(method, "AboutToShow") == 0) {
        GVariant* need_update = g_variant_new_boolean(FALSE);
        g_dbus_method_invocation_return_value(invocation, g_variant_new_tuple(&need_update, 1));
        return;
    }
    if (g_strcmp0(method, "AboutToShowGroup") == 0) {
        std::array<GVariant*, 2> out{g_variant_new_array(G_VARIANT_TYPE("i"), nullptr, 0),
                                     g_variant_new_array(G_VARIANT_TYPE("i"), nullptr, 0)};
        g_dbus_method_invocation_return_value(invocation,
                                              g_variant_new_tuple(out.data(), out.size()));
        return;
    }
    if (g_strcmp0(method, "EventGroup") == 0) {
        GVariant* id_errors = g_variant_new_array(G_VARIANT_TYPE("i"), nullptr, 0);
        g_dbus_method_invocation_return_value(invocation, g_variant_new_tuple(&id_errors, 1));
        return;
    }
    if (g_strcmp0(method, "GetProperty") == 0) {
        GVariant* value = g_variant_new_variant(g_variant_new_boolean(TRUE));
        g_dbus_method_invocation_return_value(invocation, g_variant_new_tuple(&value, 1));
        return;
    }
    g_dbus_method_invocation_return_error_literal(
        invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, "unsupported dbusmenu method");
}

GVariant* StatusNotifierItem::handle_menu_get(GDBusConnection* /*connection*/,
                                              const char* /*sender*/, const char* /*path*/,
                                              const char* /*iface*/, const char* property,
                                              GError** /*error*/, gpointer /*self_ptr*/) {
    if (g_strcmp0(property, "Version") == 0) {
        return g_variant_new_uint32(3);
    }
    if (g_strcmp0(property, "Status") == 0) {
        return g_variant_new_string("normal");
    }
    if (g_strcmp0(property, "TextDirection") == 0) {
        return g_variant_new_string("ltr");
    }
    if (g_strcmp0(property, "IconThemePath") == 0) {
        return g_variant_new_strv(nullptr, 0);
    }
    return nullptr;
}

} // namespace copyclip::ui
