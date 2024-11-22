# hotkeys.py
import threading
import time
import subprocess
from Xlib import X, XK, display
from Xlib.ext import record
from Xlib.protocol import rq

class HotkeyManager:
    def __init__(self, clipboard_manager, history_manager, ui_manager):
        self.clipboard_manager = clipboard_manager
        self.history_manager = history_manager
        self.ui_manager = ui_manager
        self.display = display.Display()
        self.root = self.display.screen().root
        self.ctx = None
        self.running = True
        
        self.ctrl_pressed = False
        self.alt_pressed = False
        
    def key_pressed(self, key):
        keycode = key.detail
        keysym = self.display.keycode_to_keysym(keycode, 0)
        if keysym == XK.XK_Control_L or keysym == XK.XK_Control_R:
            self.ctrl_pressed = True
        elif keysym == XK.XK_Super_L or keysym == XK.XK_Super_R:
            self.alt_pressed = True
        elif keysym == XK.XK_c and self.ctrl_pressed:
            time.sleep(0.1)
            content = self.clipboard_manager.check_for_new_content()
            if content:
                # Add to history
                self.history_manager.add_to_history(content)
                # Keep it as current clipboard content
                self.clipboard_manager.set_clipboard_content(content)
        elif keysym == XK.XK_v and self.alt_pressed:
            self.ui_manager.show_clipboard()
    def key_released(self, key):
        keycode = key.detail
        keysym = self.display.keycode_to_keysym(keycode, 0)
        if keysym == XK.XK_Control_L or keysym == XK.XK_Control_R:
            self.ctrl_pressed = False
        elif keysym == XK.XK_Super_L or keysym == XK.XK_Super_R:
            self.alt_pressed = False

    def handler(self, reply):
        """ Handle key events """
        if reply.category != record.FromServer:
            return
        if reply.client_swapped:
            return
        
        data = reply.data
        while len(data):
            event, data = rq.EventField(None).parse_binary_value(
                data, self.display.display, None, None)
            
            if event.type == X.KeyPress:
                self.key_pressed(event)
            elif event.type == X.KeyRelease:
                self.key_released(event)

    def setup_hotkeys(self):
        """ Setup keyboard monitoring """
        ctx = self.display.record_create_context(
            0,
            [record.AllClients],
            [{
                'core_requests': (0, 0),
                'core_replies': (0, 0),
                'ext_requests': (0, 0, 0, 0),
                'ext_replies': (0, 0, 0, 0),
                'delivered_events': (0, 0),
                'device_events': (X.KeyPress, X.KeyRelease),
                'errors': (0, 0),
                'client_started': False,
                'client_died': False,
            }]
        )
        
        threading.Thread(target=self.record_thread, args=(ctx,), daemon=True).start()

    def record_thread(self, ctx):
        """ Thread function for keyboard monitoring """
        self.display.record_enable_context(ctx, self.handler)
        self.display.record_free_context(ctx)

    def stop_listening(self):
        """ Stop keyboard monitoring """
        self.running = False
        if self.display:
            self.display.close()