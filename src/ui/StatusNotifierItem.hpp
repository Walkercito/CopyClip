#pragma once

// A panel/tray icon via the StatusNotifierItem + com.canonical.dbusmenu D-Bus
// interfaces, driven with raw Gio (libayatana-appindicator is GTK3 and would clash
// with our GTK4 process). It registers with org.kde.StatusNotifierWatcher; where no
// watcher exists — vanilla GNOME without the AppIndicator extension, or a headless
// session — it stays silently inactive rather than failing. The icon's menu is
// Show Clipboard / Settings / Quit, each routed to an injected callback.

#include <gio/gio.h>

#include <functional>
#include <string>

namespace copyclip::ui {

class StatusNotifierItem {
public:
    using Callback = std::function<void()>;

    StatusNotifierItem(std::string icon_name, Callback on_open, Callback on_settings,
                       Callback on_quit);
    ~StatusNotifierItem();

    StatusNotifierItem(const StatusNotifierItem&) = delete;
    StatusNotifierItem& operator=(const StatusNotifierItem&) = delete;
    StatusNotifierItem(StatusNotifierItem&&) = delete;
    StatusNotifierItem& operator=(StatusNotifierItem&&) = delete;

private:
    static void on_bus_acquired(GDBusConnection* connection, const char* name, gpointer self);
    static void handle_item_call(GDBusConnection* connection, const char* sender, const char* path,
                                 const char* iface, const char* method, GVariant* params,
                                 GDBusMethodInvocation* invocation, gpointer self);
    static GVariant* handle_item_get(GDBusConnection* connection, const char* sender,
                                     const char* path, const char* iface, const char* property,
                                     GError** error, gpointer self);
    static void handle_menu_call(GDBusConnection* connection, const char* sender, const char* path,
                                 const char* iface, const char* method, GVariant* params,
                                 GDBusMethodInvocation* invocation, gpointer self);
    static GVariant* handle_menu_get(GDBusConnection* connection, const char* sender,
                                     const char* path, const char* iface, const char* property,
                                     GError** error, gpointer self);

    void dispatch_menu_event(int id);

    std::string icon_name_;
    Callback on_open_;
    Callback on_settings_;
    Callback on_quit_;
    std::string bus_name_;
    GDBusConnection* connection_ = nullptr; // borrowed: the shared session bus
    GDBusNodeInfo* item_node_ = nullptr;
    GDBusNodeInfo* menu_node_ = nullptr;
    guint owner_id_ = 0;
    guint item_registration_ = 0;
    guint menu_registration_ = 0;
};

} // namespace copyclip::ui
