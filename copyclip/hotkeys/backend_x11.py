"""X11 hotkey backend using Xlib RECORD extension."""

import threading
import time
import traceback
from typing import TYPE_CHECKING

from PyQt6.QtCore import QObject, pyqtSignal
from Xlib import XK, X, display
from Xlib.ext import record
from Xlib.protocol import rq

from copyclip.hotkeys.backend_base import HotkeyBackend
from copyclip.hotkeys.config import HotkeyConfig

if TYPE_CHECKING:
    from copyclip.core.clipboard import ClipboardManager
    from copyclip.core.history import HistoryManager
    from copyclip.hotkeys.config import HotkeyBinding
    from copyclip.ui.main_window import MainWindow


class X11HotkeySignals(QObject):
    """Qt signals for X11 hotkey events."""

    show_clipboard_signal = pyqtSignal()
    content_copied_signal = pyqtSignal(str)


class X11HotkeyBackend(HotkeyBackend):
    """X11 hotkey backend using Xlib RECORD extension for global keyboard monitoring."""

    def __init__(
        self,
        clipboard_manager: "ClipboardManager",
        history_manager: "HistoryManager",
        ui_manager: "MainWindow",
        binding: "HotkeyBinding",
    ):
        """Initialize the X11 hotkey backend.

        Args:
            clipboard_manager: ClipboardManager instance
            history_manager: HistoryManager instance
            ui_manager: Main window instance
            binding: Hotkey binding configuration
        """
        self.clipboard_manager = clipboard_manager
        self.history_manager = history_manager
        self.ui_manager = ui_manager
        self.binding = binding

        # X11 setup
        try:
            self.display = display.Display()
        except Exception as e:
            print(f"Error initializing X11 display: {e}")
            raise

        self.ctx = None
        self.running = True
        self.debug = False

        # Track modifier key states
        self.modifier_states = {
            "ctrl": False,
            "alt": False,
            "super": False,
            "shift": False,
        }

        # Setup Qt signals
        self.signals = X11HotkeySignals()
        self.signals.show_clipboard_signal.connect(self.ui_manager.show_clipboard)
        self.signals.content_copied_signal.connect(self._handle_copied_content)

    def _handle_copied_content(self, content: str) -> None:
        """Handle content that was copied to clipboard.

        Args:
            content: The copied content
        """
        if content:
            self.history_manager.add_item(content)
            self.clipboard_manager.set_content(content)

    def _check_modifier_pressed(self, modifier: str) -> bool:
        """Check if a modifier (or combination) is pressed.

        Args:
            modifier: Modifier name (e.g., 'ctrl', 'super', 'ctrl+alt')

        Returns:
            True if the modifier(s) are pressed
        """
        if "+" in modifier:
            parts = modifier.split("+")
            return all(self.modifier_states.get(part, False) for part in parts)
        return self.modifier_states.get(modifier, False)

    def _check_hotkey_match(self, keysym: int) -> bool:
        """Check if the pressed key matches the configured hotkey.

        Args:
            keysym: X11 keysym of the pressed key

        Returns:
            True if the hotkey matches
        """
        expected_keysym = HotkeyConfig.get_key_keysym(self.binding.key)
        if expected_keysym is None:
            return False

        return keysym == expected_keysym and self._check_modifier_pressed(self.binding.modifier)

    def _update_modifier_state(self, keysym: int, pressed: bool) -> None:
        """Update the state of modifier keys.

        Args:
            keysym: X11 keysym of the key
            pressed: True if key was pressed, False if released
        """
        if keysym in [XK.XK_Control_L, XK.XK_Control_R]:
            self.modifier_states["ctrl"] = pressed
        elif keysym in [XK.XK_Alt_L, XK.XK_Alt_R]:
            self.modifier_states["alt"] = pressed
        elif keysym in [XK.XK_Super_L, XK.XK_Super_R]:
            self.modifier_states["super"] = pressed
        elif keysym in [XK.XK_Shift_L, XK.XK_Shift_R]:
            self.modifier_states["shift"] = pressed

    def _key_pressed(self, key) -> None:
        """Handle key press events.

        Args:
            key: X11 key event
        """
        keycode = key.detail
        keysym = self.display.keycode_to_keysym(keycode, 0)

        if self.debug:
            try:
                keyname = XK.keysym_to_string(keysym) or f"keysym_{keysym}"
            except Exception:
                keyname = f"keysym_{keysym}"
            print(f"KEY PRESS: {keyname} (keysym={keysym}), modifiers: {self.modifier_states}")

        self._update_modifier_state(keysym, True)

        # Handle Ctrl+C for clipboard copy detection
        if keysym == XK.XK_c and self.modifier_states["ctrl"]:
            time.sleep(0.1)
            content = self.clipboard_manager.check_for_changes()
            if content:
                self.signals.content_copied_signal.emit(content)

        if self._check_hotkey_match(keysym):
            if self.debug:
                print("HOTKEY MATCH! Emitting show_clipboard signal")
            self.signals.show_clipboard_signal.emit()

    def _key_released(self, key) -> None:
        """Handle key release events.

        Args:
            key: X11 key event
        """
        keycode = key.detail
        keysym = self.display.keycode_to_keysym(keycode, 0)
        self._update_modifier_state(keysym, False)

    def _handler(self, reply) -> None:
        """Handle X11 record events.

        Args:
            reply: X11 record reply
        """
        if reply.category != record.FromServer or reply.client_swapped:
            return

        data = reply.data
        while len(data):
            try:
                event, data = rq.EventField(None).parse_binary_value(
                    data, self.display.display, None, None
                )

                if event.type == X.KeyPress:
                    self._key_pressed(event)
                elif event.type == X.KeyRelease:
                    self._key_released(event)
            except Exception as e:
                print(f"Error handling keyboard event: {e}")
                break

    def setup(self) -> bool:
        """Setup keyboard monitoring.

        Returns:
            True if setup was successful, False otherwise
        """
        try:
            thread = threading.Thread(target=self._record_thread, daemon=True)
            thread.start()
            time.sleep(0.1)
            return True

        except Exception as e:
            print(f"ERROR: Failed to setup X11 hotkeys: {e}")
            print(
                "\nThis may be due to:"
                "\n  1. Missing X11 record extension permissions"
                "\n  2. Another application using X11 record extension"
            )
            return False

    def _record_thread(self) -> None:
        """Thread function for keyboard monitoring."""
        if self.debug:
            print("X11 record thread started")

        try:
            ctx = self.display.record_create_context(
                0,
                [record.AllClients],
                [
                    {
                        "core_requests": (0, 0),
                        "core_replies": (0, 0),
                        "ext_requests": (0, 0, 0, 0),
                        "ext_replies": (0, 0, 0, 0),
                        "delivered_events": (0, 0),
                        "device_events": (X.KeyPress, X.KeyRelease),
                        "errors": (0, 0),
                        "client_started": False,
                        "client_died": False,
                    }
                ],
            )

            if self.debug:
                print("Record context created, enabling...")

            self.display.record_enable_context(ctx, self._handler)

            if self.debug:
                print("Record context ended")

            self.display.record_free_context(ctx)

        except Exception as e:
            print(f"ERROR: Exception in X11 record thread: {e}")
            traceback.print_exc()

    def stop(self) -> None:
        """Stop keyboard monitoring."""
        self.running = False
        if self.display:
            try:
                self.display.close()
            except Exception as e:
                print(f"Error closing display: {e}")

    def update_hotkey(self, binding: "HotkeyBinding") -> bool:
        """Update the hotkey binding.

        Args:
            binding: New hotkey binding

        Returns:
            True if update was successful
        """
        self.binding = binding
        print(f"Hotkey updated to: {self.binding.display_name}")
        return True

    def enable_debug(self) -> None:
        """Enable debug logging."""
        self.debug = True
        print("Debug logging enabled (X11 backend)")
