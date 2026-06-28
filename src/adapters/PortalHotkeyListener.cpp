#include "adapters/PortalHotkeyListener.hpp"

#include "config/Constants.hpp"

#include <spdlog/spdlog.h>

#include <QDBusInterface>
#include <QDBusMessage>
#include <QVariant>

#include <stdexcept>
#include <string>
#include <utility>

namespace copyclip::adapters {

namespace {

const QString kPortalService = QStringLiteral("org.freedesktop.portal.Desktop");
const QString kPortalPath = QStringLiteral("/org/freedesktop/portal/desktop");
const QString kGlobalShortcutsIface = QStringLiteral("org.freedesktop.portal.GlobalShortcuts");
const QString kShortcutId = QStringLiteral("show-clipboard");
const QString kActivatedSignal = QStringLiteral("Activated");

} // namespace

PortalHotkeyListener::PortalHotkeyListener(core::HotkeySpec spec)
    : spec_{std::move(spec)}, bus_{QDBusConnection::sessionBus()} {
    interface_ =
        std::make_unique<QDBusInterface>(kPortalService, kPortalPath, kGlobalShortcutsIface, bus_);
    if (!interface_->isValid()) {
        throw std::runtime_error{"GlobalShortcuts portal is unavailable"};
    }
}

PortalHotkeyListener::~PortalHotkeyListener() = default;

void PortalHotkeyListener::start(std::function<void()> on_activate) {
    on_activate_ = std::move(on_activate);
    bus_.connect(kPortalService, kPortalPath, kGlobalShortcutsIface, kActivatedSignal, this,
                 SLOT(handle_activated(QDBusObjectPath, QString, qulonglong, QVariantMap)));
    create_session();
}

void PortalHotkeyListener::stop() {
    on_activate_ = nullptr;
}

bool PortalHotkeyListener::rebind(const core::HotkeySpec& spec) {
    spec_ = spec;
    create_session(); // re-binding through the portal needs a fresh round-trip
    return true;
}

void PortalHotkeyListener::create_session() {
    const QString token = QString::fromStdString(std::string{config::kAppId}) + "_session";
    QVariantMap options;
    options.insert(QStringLiteral("session_handle_token"), token);
    options.insert(QStringLiteral("handle_token"), token);
    const QDBusMessage reply = interface_->call(QStringLiteral("CreateSession"), options);
    spdlog::debug("portal CreateSession for {}: {} reply arg(s)", spec_.display_name(),
                  reply.arguments().size());
    // BindShortcuts is issued from the CreateSession response handler in a full
    // implementation; left as the iteration point for real hardware.
}

void PortalHotkeyListener::handle_activated(const QDBusObjectPath& /*session_handle*/,
                                            const QString& shortcut_id, qulonglong /*timestamp*/,
                                            const QVariantMap& /*options*/) {
    if (shortcut_id == kShortcutId && on_activate_) {
        on_activate_();
    }
}

} // namespace copyclip::adapters
