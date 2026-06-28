#pragma once

// X11 global hotkey via a targeted XGrabKey on the root window (not a
// system-wide keylogger). Mirrors the reference adapters/hotkeys/x11.py.
//
// Construction opens the X display and throws if none is available, letting the
// factory fall back to the manual listener. start() grabs the combo (plus its
// NumLock/CapsLock variants) and serves events on a background thread that shuts
// down promptly via a poll() loop driven by the jthread stop token — cleaner
// than the reference's close-the-display approach.
//
// MANUAL VERIFICATION: the grab + event loop need a real X server and physical
// key presses, so they are verified by hand (press the combo -> activation
// fires); only the pure modifiers_to_mask() seam is unit tested. Xlib is touched
// from the constructing thread (grab) and the serve thread (event read), so
// XInitThreads() is requested (best-effort) to enable Xlib's internal locking.

#include "adapters/X11Modifiers.hpp"
#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <X11/Xlib.h>

#include <functional>
#include <memory>
#include <stop_token>
#include <thread>

namespace copyclip::adapters {

class X11HotkeyListener final : public core::HotkeyListener {
public:
    // Throws std::runtime_error if no X display can be opened.
    explicit X11HotkeyListener(core::HotkeySpec spec);
    ~X11HotkeyListener() override = default;

    X11HotkeyListener(const X11HotkeyListener&) = delete;
    X11HotkeyListener& operator=(const X11HotkeyListener&) = delete;
    X11HotkeyListener(X11HotkeyListener&&) = delete;
    X11HotkeyListener& operator=(X11HotkeyListener&&) = delete;

    void start(std::function<void()> on_activate) override;
    void stop() override;
    bool rebind(const core::HotkeySpec& spec) override;

private:
    void serve(const std::stop_token& stop_token);
    void grab();
    void ungrab();
    [[nodiscard]] int keycode() const;

    struct DisplayCloser {
        void operator()(Display* display) const noexcept;
    };
    using DisplayPtr = std::unique_ptr<Display, DisplayCloser>;

    core::HotkeySpec spec_;
    std::function<void()> on_activate_;
    DisplayPtr display_;
    Window root_ = 0;
    std::jthread thread_;
};

} // namespace copyclip::adapters
