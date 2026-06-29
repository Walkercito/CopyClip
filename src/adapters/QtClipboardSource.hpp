#pragma once

// Clipboard source backed by QClipboard (event-driven on X11 and Wayland).
// Mirrors the reference adapters/clipboard/qt.py.
//
// Requires a QGuiApplication before construction (it owns the clipboard). The
// dataChanged subscription is a non-QObject connection, torn down in stop() and
// the destructor.

#include "core/Interfaces.hpp"

#include <QClipboard>
#include <QMetaObject>

#include <functional>
#include <optional>
#include <string>

namespace copyclip::adapters {

class QtClipboardSource final : public core::ClipboardSource {
public:
    // Throws std::runtime_error if no QGuiApplication has been created yet.
    QtClipboardSource();
    ~QtClipboardSource() override;

    QtClipboardSource(const QtClipboardSource&) = delete;
    QtClipboardSource& operator=(const QtClipboardSource&) = delete;
    QtClipboardSource(QtClipboardSource&&) = delete;
    QtClipboardSource& operator=(QtClipboardSource&&) = delete;

    void start(std::function<void(const core::ClipContent&)> on_change) override;
    void stop() override;
    [[nodiscard]] std::optional<std::string> read() const override;
    bool write(const core::ClipContent& content) override;

private:
    void handle_change();

    QClipboard* clipboard_ = nullptr;
    std::function<void(const core::ClipContent&)> on_change_;
    QMetaObject::Connection connection_;
};

} // namespace copyclip::adapters
