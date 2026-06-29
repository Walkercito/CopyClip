#include "adapters/QtClipboardSource.hpp"

#include <QGuiApplication>
#include <QString>

#include <stdexcept>
#include <utility>

namespace copyclip::adapters {

QtClipboardSource::QtClipboardSource() {
    if (QGuiApplication::instance() == nullptr) {
        throw std::runtime_error{"QGuiApplication must be created before QtClipboardSource"};
    }
    clipboard_ = QGuiApplication::clipboard();
}

QtClipboardSource::~QtClipboardSource() {
    QObject::disconnect(connection_);
}

void QtClipboardSource::start(std::function<void(const core::ClipContent&)> on_change) {
    on_change_ = std::move(on_change);
    connection_ =
        QObject::connect(clipboard_, &QClipboard::dataChanged, [this] { handle_change(); });
}

void QtClipboardSource::stop() {
    QObject::disconnect(connection_);
    on_change_ = nullptr;
}

std::optional<std::string> QtClipboardSource::read() const {
    const QString text = clipboard_->text();
    if (text.isEmpty()) {
        return std::nullopt;
    }
    return text.toStdString();
}

bool QtClipboardSource::write(const core::ClipContent& content) {
    // Legacy Qt path: text only (the GTK app carries rich text and images).
    clipboard_->setText(QString::fromStdString(content.text));
    return true;
}

void QtClipboardSource::handle_change() {
    const QString text = clipboard_->text();
    if (!text.isEmpty() && on_change_) {
        on_change_(core::ClipContent{.kind = core::ClipKind::Text, .text = text.toStdString()});
    }
}

} // namespace copyclip::adapters
