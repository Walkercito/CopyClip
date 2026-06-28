#pragma once

// Wayland global hotkey via the XDG Desktop Portal GlobalShortcuts interface
// (Qt6::DBus). Mirrors the reference adapters/hotkeys/wayland_portal.py.
//
// The portal owns the actual key binding (the user confirms it in a system
// dialog); this class creates a session, subscribes to the portal's Activated
// signal, and forwards it to the activation callback. Construction throws if the
// GlobalShortcuts portal is unavailable, so the factory can fall back.
//
// INITIAL VERSION / MANUAL VERIFICATION: the async CreateSession ->
// BindShortcuts -> Activated flow needs a portal-capable Wayland session and
// real hardware to iterate. BindShortcuts is issued from the CreateSession
// response handler in a complete implementation; that is the iteration point
// left for real hardware. There is no automated test (mirroring the reference);
// verify by hand on a portal-capable session.

#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include <functional>
#include <memory>

class QDBusInterface;

namespace copyclip::adapters {

class PortalHotkeyListener : public QObject, public core::HotkeyListener {
    Q_OBJECT

public:
    // Throws std::runtime_error if the GlobalShortcuts portal is unavailable.
    explicit PortalHotkeyListener(core::HotkeySpec spec);
    ~PortalHotkeyListener() override;

    PortalHotkeyListener(const PortalHotkeyListener&) = delete;
    PortalHotkeyListener& operator=(const PortalHotkeyListener&) = delete;
    PortalHotkeyListener(PortalHotkeyListener&&) = delete;
    PortalHotkeyListener& operator=(PortalHotkeyListener&&) = delete;

    void start(std::function<void()> on_activate) override;
    void stop() override;
    bool rebind(const core::HotkeySpec& spec) override;

private:
    void create_session();

    core::HotkeySpec spec_;
    std::function<void()> on_activate_;
    QDBusConnection bus_;
    std::unique_ptr<QDBusInterface> interface_;

public Q_SLOTS:
    // Forwards the portal's Activated signal to the activation callback. Public
    // because it is invoked by the D-Bus signal/slot machinery.
    void handle_activated(const QDBusObjectPath& session_handle, const QString& shortcut_id,
                          qulonglong timestamp, const QVariantMap& options);
};

} // namespace copyclip::adapters
