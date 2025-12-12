"""Hotkey configuration management."""

from dataclasses import dataclass

from Xlib import XK


@dataclass
class HotkeyBinding:
    """Represents a keyboard shortcut binding."""

    modifier: str  # 'ctrl', 'alt', 'super', 'shift', or combined like 'ctrl+alt'
    key: str  # Key name like 'v', 'c', etc.
    display_name: str  # Human-readable name like "Super+V"

    def __str__(self) -> str:
        return self.display_name


class HotkeyConfig:
    """Maps hotkey configurations to Xlib keysyms."""

    # Available hotkey presets
    PRESETS = {
        "super_v": HotkeyBinding("super", "v", "Super+V"),
        "ctrl_alt_v": HotkeyBinding("ctrl+alt", "v", "Ctrl+Alt+V"),
        "super_c": HotkeyBinding("super", "c", "Super+C"),
        "ctrl_shift_v": HotkeyBinding("ctrl+shift", "v", "Ctrl+Shift+V"),
    }

    # Modifier key mappings to X11 keysyms
    MODIFIER_KEYSYMS = {
        "ctrl": [XK.XK_Control_L, XK.XK_Control_R],
        "alt": [XK.XK_Alt_L, XK.XK_Alt_R],
        "super": [XK.XK_Super_L, XK.XK_Super_R],
        "shift": [XK.XK_Shift_L, XK.XK_Shift_R],
    }

    # Key mappings to X11 keysyms
    KEY_KEYSYMS = {
        "v": XK.XK_v,
        "c": XK.XK_c,
        "x": XK.XK_x,
        "space": XK.XK_space,
    }

    @classmethod
    def get_binding(cls, preset_name: str) -> HotkeyBinding | None:
        """Get a hotkey binding by preset name.

        Args:
            preset_name: Name of the preset (e.g., 'super_v')

        Returns:
            HotkeyBinding or None if not found
        """
        return cls.PRESETS.get(preset_name)

    @classmethod
    def get_default_binding(cls) -> HotkeyBinding:
        """Get the default hotkey binding.

        Returns:
            The default hotkey binding (Super+V)
        """
        return cls.PRESETS["super_v"]

    @classmethod
    def get_modifier_keysyms(cls, modifier: str) -> list[int]:
        """Get Xlib keysyms for a modifier.

        Args:
            modifier: Modifier name (e.g., 'ctrl', 'super', 'ctrl+alt')

        Returns:
            List of keysym codes for the modifier
        """
        if "+" in modifier:
            parts = modifier.split("+")
            keysyms = []
            for part in parts:
                keysyms.extend(cls.MODIFIER_KEYSYMS.get(part, []))
            return keysyms
        return cls.MODIFIER_KEYSYMS.get(modifier, [])

    @classmethod
    def get_key_keysym(cls, key: str) -> int | None:
        """Get Xlib keysym for a key.

        Args:
            key: Key name (e.g., 'v', 'c')

        Returns:
            Keysym code or None if not found
        """
        return cls.KEY_KEYSYMS.get(key)

    @classmethod
    def get_all_presets(cls) -> dict[str, HotkeyBinding]:
        """Get all available preset bindings.

        Returns:
            Dictionary of preset name to HotkeyBinding
        """
        return cls.PRESETS.copy()
