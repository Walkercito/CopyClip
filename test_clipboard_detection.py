#!/usr/bin/env python3
"""Test clipboard detection."""

import time

from copyclip.core.clipboard import ClipboardManager

print("Testing clipboard monitoring...")
cm = ClipboardManager()

print("Current clipboard:", repr(cm.get_content()[:50] if cm.get_content() else None))
print("\nCopy something with Ctrl+C and wait...")

for i in range(10):
    time.sleep(1)
    content = cm.check_for_changes()
    if content:
        print(f"[{i + 1}] ✓ Detected change: {repr(content[:50])}")
    else:
        print(f"[{i + 1}] No change detected")

print("\nDone")
