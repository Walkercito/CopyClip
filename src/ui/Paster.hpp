#pragma once

// Abstract paste capability: simulate pasting the clipboard into the focused
// window. Injected so the copy use case can be exercised without spawning input
// tools (see KeystrokePaster for the real implementation).

namespace copyclip::ui {

class Paster {
public:
    Paster() = default;
    virtual ~Paster() = default;

    Paster(const Paster&) = delete;
    Paster& operator=(const Paster&) = delete;
    Paster(Paster&&) = delete;
    Paster& operator=(Paster&&) = delete;

    virtual void paste() const = 0;
};

} // namespace copyclip::ui
