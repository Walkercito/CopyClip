#include "ui/CopyAction.hpp"

#include "core/Models.hpp"

#include <glibmm/main.h>

namespace copyclip::ui {

namespace {

constexpr unsigned int kPasteDelayMs = 120;

} // namespace

CopyAction::CopyAction(core::ClipboardSource& clipboard, core::HistoryService& history,
                       core::SettingsService& settings, Paster& paster)
    : clipboard_{clipboard}, history_{history}, settings_{settings}, paster_{paster} {}

CopyAction::~CopyAction() {
    paste_connection_.disconnect();
}

bool CopyAction::run(const std::string& content) {
    clipboard_.get().write(content);
    history_.get().add(content);
    const core::Settings& settings = settings_.get().settings();
    if (settings.auto_paste) {
        // Cancel any still-pending paste, then schedule one for after focus returns.
        // A connection (not connect_once) so a pending paste can be disconnected on
        // destruction; the slot returns false to run exactly once.
        paste_connection_.disconnect();
        paste_connection_ = Glib::signal_timeout().connect(
            [this] {
                paster_.get().paste();
                return false;
            },
            kPasteDelayMs);
    }
    return settings.auto_hide_on_copy || settings.auto_paste;
}

} // namespace copyclip::ui
