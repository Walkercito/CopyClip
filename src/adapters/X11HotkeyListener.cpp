#include "adapters/X11HotkeyListener.hpp"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <poll.h>

#include <array>
#include <cerrno>
#include <stdexcept>
#include <string>
#include <utility>

namespace copyclip::adapters {

namespace {

// How long the event loop blocks in poll() before re-checking the stop token.
constexpr int kPollTimeoutMs = 100;

// Lock-modifier states to also grab, so the binding matches regardless of
// NumLock (Mod2) / CapsLock (Lock).
constexpr std::array<unsigned int, 4> kLockVariants{
    0U,
    static_cast<unsigned int>(LockMask),
    static_cast<unsigned int>(Mod2Mask),
    static_cast<unsigned int>(LockMask | Mod2Mask),
};

unsigned int modifier_bit(core::Modifier modifier) {
    switch (modifier) {
    case core::Modifier::Ctrl:
        return static_cast<unsigned int>(ControlMask);
    case core::Modifier::Alt:
        return static_cast<unsigned int>(Mod1Mask);
    case core::Modifier::Super:
        return static_cast<unsigned int>(Mod4Mask);
    case core::Modifier::Shift:
        return static_cast<unsigned int>(ShiftMask);
    }
    return 0U;
}

} // namespace

unsigned int modifiers_to_mask(std::span<const core::Modifier> modifiers) {
    unsigned int mask = 0U;
    for (const core::Modifier modifier : modifiers) {
        mask |= modifier_bit(modifier);
    }
    return mask;
}

void X11HotkeyListener::DisplayCloser::operator()(Display* display) const noexcept {
    if (display != nullptr) {
        XCloseDisplay(display);
    }
}

X11HotkeyListener::X11HotkeyListener(core::HotkeySpec spec) : spec_{std::move(spec)} {
    XInitThreads(); // best-effort: enable Xlib's internal locking
    display_ = DisplayPtr{XOpenDisplay(nullptr)};
    if (!display_) {
        throw std::runtime_error{"could not open X display"};
    }
    root_ = DefaultRootWindow(display_.get());
}

void X11HotkeyListener::start(std::function<void()> on_activate) {
    on_activate_ = std::move(on_activate);
    grab();
    thread_ = std::jthread{[this](const std::stop_token& stop_token) { serve(stop_token); }};
}

void X11HotkeyListener::stop() {
    if (thread_.joinable()) {
        thread_.request_stop();
        thread_.join();
    }
    ungrab();
}

bool X11HotkeyListener::rebind(const core::HotkeySpec& spec) {
    ungrab();
    spec_ = spec;
    grab();
    return true;
}

int X11HotkeyListener::keycode() const {
    const std::string key_name{to_string(spec_.key)};
    const KeySym keysym = XStringToKeysym(key_name.c_str());
    return XKeysymToKeycode(display_.get(), keysym);
}

void X11HotkeyListener::grab() {
    const int code = keycode();
    const unsigned int base = modifiers_to_mask(spec_.modifiers);
    for (const unsigned int lock : kLockVariants) {
        XGrabKey(display_.get(), code, base | lock, root_, True, GrabModeAsync, GrabModeAsync);
    }
    XSync(display_.get(), False);
}

void X11HotkeyListener::ungrab() {
    const int code = keycode();
    const unsigned int base = modifiers_to_mask(spec_.modifiers);
    for (const unsigned int lock : kLockVariants) {
        XUngrabKey(display_.get(), code, base | lock, root_);
    }
    XSync(display_.get(), False);
}

void X11HotkeyListener::serve(const std::stop_token& stop_token) {
    const int connection = ConnectionNumber(display_.get());
    while (!stop_token.stop_requested()) {
        pollfd descriptor{};
        descriptor.fd = connection;
        descriptor.events = POLLIN;

        const int ready = ::poll(&descriptor, 1, kPollTimeoutMs);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (ready == 0) {
            continue;
        }
        while (XPending(display_.get()) > 0) {
            XEvent event{};
            XNextEvent(display_.get(), &event);
            if (event.type == KeyPress && on_activate_) {
                on_activate_();
            }
        }
    }
}

} // namespace copyclip::adapters
