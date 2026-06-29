#pragma once

// Concrete Paster that simulates Ctrl+V via the session's input tool (xdotool on
// X11; wtype then ydotool on Wayland). Best-effort: a no-op if no tool succeeds —
// the clip is on the clipboard either way, so the user can still paste manually.

#include "core/Enums.hpp"
#include "ui/Paster.hpp"

namespace copyclip::ui {

class KeystrokePaster final : public Paster {
public:
    explicit KeystrokePaster(core::SessionType session);

    void paste() const override;

private:
    core::SessionType session_;
};

} // namespace copyclip::ui
