#pragma once

// Clipboard source backed by Qt's QClipboard — event-driven on both X11 and
// Wayland. Mirrors the reference adapters/clipboard/qt.py.
//
// Requires a QGuiApplication to exist before construction (the clipboard is
// owned by the GUI application). Part of the impure adapters layer; links
// Qt6::Gui. The dataChanged subscription is a non-QObject connection, torn down
// in stop() and the destructor.

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

    void start(std::function<void(const std::string&)> on_change) override;
    void stop() override;
    [[nodiscard]] std::optional<std::string> read() const override;
    void write(const std::string& text) override;

private:
    void handle_change();

    QClipboard* clipboard_ = nullptr;
    std::function<void(const std::string&)> on_change_;
    QMetaObject::Connection connection_;
};

} // namespace copyclip::adapters
