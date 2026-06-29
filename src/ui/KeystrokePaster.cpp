#include "ui/KeystrokePaster.hpp"

#include <glibmm/error.h>
#include <glibmm/miscutils.h>
#include <glibmm/spawn.h>

#include <spdlog/spdlog.h>

#include <fcntl.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace copyclip::ui {

namespace {

// Timings mirror the reference: the compositor needs a moment to notice the new
// virtual device, and a gap between key events so each registers.
constexpr auto kDeviceSettle = std::chrono::milliseconds(100);
constexpr auto kKeyEventDelay = std::chrono::milliseconds(50);

// Arbitrary identifiers for the synthetic device (only used for naming/debug).
constexpr std::uint16_t kVendorId = 0x1234;
constexpr std::uint16_t kProductId = 0x5678;
constexpr std::uint16_t kDeviceVersion = 1;
constexpr const char* kDeviceName = "copyclip-paste";

// input_event values: a key's pressed/released state, and the SYN_REPORT marker.
constexpr std::int32_t kKeyPress = 1;
constexpr std::int32_t kKeyRelease = 0;
constexpr std::int32_t kSynValue = 0;

// Driving /dev/uinput means C open()/ioctl() varargs and a flat type/code/value
// event struct, which trip pro-type-vararg and easily-swappable-parameters — rules
// that cannot be met for raw kernel syscalls. Suppressed for just this block.
// NOLINTBEGIN(cppcoreguidelines-pro-type-vararg, bugprone-easily-swappable-parameters)

// RAII for the transient uinput virtual keyboard: closes the fd, and destroys the
// device first if it was created.
class UinputKeyboard {
public:
    UinputKeyboard() : fd_{::open("/dev/uinput", O_WRONLY | O_CLOEXEC)} {}
    ~UinputKeyboard() {
        if (created_) {
            ::ioctl(fd_, UI_DEV_DESTROY);
        }
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    UinputKeyboard(const UinputKeyboard&) = delete;
    UinputKeyboard& operator=(const UinputKeyboard&) = delete;
    UinputKeyboard(UinputKeyboard&&) = delete;
    UinputKeyboard& operator=(UinputKeyboard&&) = delete;

    [[nodiscard]] bool opened() const { return fd_ >= 0; }
    [[nodiscard]] int fd() const { return fd_; }
    void mark_created() { created_ = true; }

private:
    int fd_;
    bool created_ = false;
};

// Write one input_event; true on a complete write.
[[nodiscard]] bool emit(int fd, std::uint16_t type, std::uint16_t code, std::int32_t value) {
    input_event event{};
    event.type = type;
    event.code = code;
    event.value = value;
    return ::write(fd, &event, sizeof(event)) == static_cast<ssize_t>(sizeof(event));
}

// Inject Ctrl+V through a transient /dev/uinput virtual keyboard — the session-
// agnostic path the reference uses: kernel-level input that reaches X11 and Wayland
// apps alike (a terminal included). Needs write access to /dev/uinput (e.g. being
// in the `input` group); returns false otherwise so the caller can fall back.
[[nodiscard]] bool paste_via_uinput() {
    UinputKeyboard device;
    if (!device.opened()) {
        return false;
    }
    const int fd = device.fd();
    if (::ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0 || ::ioctl(fd, UI_SET_KEYBIT, KEY_LEFTCTRL) < 0 ||
        ::ioctl(fd, UI_SET_KEYBIT, KEY_V) < 0) {
        return false;
    }
    uinput_setup setup{};
    setup.id.bustype = BUS_USB;
    setup.id.vendor = kVendorId;
    setup.id.product = kProductId;
    setup.id.version = kDeviceVersion;
    std::strncpy(static_cast<char*>(setup.name), kDeviceName, sizeof(setup.name) - 1);
    if (::ioctl(fd, UI_DEV_SETUP, &setup) < 0 || ::ioctl(fd, UI_DEV_CREATE) < 0) {
        return false;
    }
    device.mark_created();
    std::this_thread::sleep_for(kDeviceSettle);

    // Press Ctrl, press V, release V, release Ctrl — each followed by a SYN report.
    const std::array<std::pair<std::uint16_t, std::int32_t>, 4> strokes = {
        {{KEY_LEFTCTRL, kKeyPress},
         {KEY_V, kKeyPress},
         {KEY_V, kKeyRelease},
         {KEY_LEFTCTRL, kKeyRelease}}};
    bool ok = true;
    for (const auto& [code, value] : strokes) {
        ok = ok && emit(fd, EV_KEY, code, value) && emit(fd, EV_SYN, SYN_REPORT, kSynValue);
        std::this_thread::sleep_for(kKeyEventDelay);
    }
    return ok;
}
// NOLINTEND(cppcoreguidelines-pro-type-vararg, bugprone-easily-swappable-parameters)

// Run `argv` (program resolved on PATH) and report a clean exit; false when the
// program is missing or fails.
[[nodiscard]] bool try_run(const std::vector<std::string>& argv) {
    if (Glib::find_program_in_path(argv.front()).empty()) {
        return false;
    }
    int wait_status = 0;
    try {
        Glib::spawn_sync("", argv, Glib::SpawnFlags::SEARCH_PATH, {}, nullptr, nullptr,
                         &wait_status);
    } catch (const Glib::Error&) {
        return false;
    }
    return wait_status == 0;
}

// CLI fallbacks, tried only if /dev/uinput is unavailable. Wayland prefers the
// virtual keyboard (wtype) then uinput-via-daemon (ydotool); X11 uses XTEST.
[[nodiscard]] std::vector<std::vector<std::string>> paste_commands(core::SessionType session) {
    if (session == core::SessionType::Wayland) {
        return {{"wtype", "-M", "ctrl", "v", "-m", "ctrl"}, {"ydotool", "key", "ctrl+v"}};
    }
    return {{"xdotool", "key", "--clearmodifiers", "ctrl+v"}};
}

} // namespace

KeystrokePaster::KeystrokePaster(core::SessionType session) : session_{session} {}

void KeystrokePaster::paste() const {
    // Inject on a detached thread: the uinput sequence sleeps for the device-settle
    // and inter-key delays (~0.3s), which must never block the UI thread. The
    // captured values are self-contained, so the thread is safe to outlive this call
    // — including the logger, captured as a shared_ptr so it (and its sinks) stay
    // alive even if the app quits and tears down spdlog's registry mid-paste.
    const core::SessionType session = session_;
    const std::shared_ptr<spdlog::logger> logger = spdlog::default_logger();
    std::thread([session, logger] {
        if (paste_via_uinput()) {
            logger->debug("auto-paste: Ctrl+V injected via /dev/uinput");
            return;
        }
        logger->warn("auto-paste: uinput unavailable/failed; trying CLI fallbacks");
        for (const std::vector<std::string>& argv : paste_commands(session)) {
            if (try_run(argv)) {
                logger->debug("auto-paste: Ctrl+V injected via {}", argv.front());
                return;
            }
        }
        logger->warn("auto-paste: no working input method; clip left on the clipboard");
    }).detach();
}

} // namespace copyclip::ui
