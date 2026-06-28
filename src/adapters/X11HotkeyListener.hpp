#pragma once

// X11 global hotkey via a targeted XGrabKey on the root window (no system-wide
// keylogger). Mirrors the reference adapters/hotkeys/x11.py.
//
// Construction opens the X display and throws if none is available, so the
// factory can fall back to the manual listener. start() grabs the combination
// (with the NumLock/CapsLock variants) and watches for it on a background
// thread; the thread shuts down promptly via a poll() loop over the X
// connection driven by the jthread stop token.
//
// MANUAL VERIFICATION: the grab + event loop need a real X server and physical
// key presses, so they are exercised by hand on an X11 session (press the combo
// -> the activation fires). Only the pure modifiers_to_mask() seam below is unit
// tested. Xlib is accessed from the constructing thread (grab) and the serve
// thread (event read); XInitThreads() is requested to enable Xlib's internal
// locking for that low-contention pattern.

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
