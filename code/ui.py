# ui.py
import customtkinter as ctk
from datetime import datetime
import json
import os

class UIManager:
    def __init__(self, history_manager):
        self.history_manager = history_manager
        self.window_pinned = False
        self.pin_button = None 
        
        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("dark-blue")
        
        self.root = ctk.CTk()
        self.root.title("CopyClip")
        self.root.geometry("300x500")
        
        self.root.grid_columnconfigure(0, weight=1)
        self.root.grid_rowconfigure(1, weight=1)
        
        self.settings = {}
        self.create_gui()
        self.load_settings()
        
        self.root.withdraw()
        
        self.root.protocol("WM_DELETE_WINDOW", self.hide_clipboard)
        self.root.bind('<Escape>', lambda e: self.hide_clipboard())
        self.root.after(1000, self.check_clipboard_updates)

    def create_gui(self):
        search_frame = ctk.CTkFrame(self.root, height=40)
        search_frame.grid(row=0, column=0, padx=10, pady=(10, 5), sticky="ew")
        search_frame.grid_columnconfigure(0, weight=1)
        
        self.search_var = ctk.StringVar()
        self.search_entry = ctk.CTkEntry(
            search_frame,
            placeholder_text="Search clips...",
            height=32,
            textvariable=self.search_var
        )
        self.search_entry.grid(row=0, column=0, padx=5, pady=5, sticky="ew")
        self.search_var.trace('w', lambda *args: self.filter_clips())
        
        clear_button = ctk.CTkButton(
            search_frame,
            text="×",
            width=32,
            height=32,
            command=self.clear_search
        )
        clear_button.grid(row=0, column=1, padx=5, pady=5)
        
        self.clips_frame = ctk.CTkScrollableFrame(self.root)
        self.clips_frame.grid(row=1, column=0, padx=10, pady=(5, 10), sticky="nsew")
        self.clips_frame.grid_columnconfigure(0, weight=1)
        
        button_frame = ctk.CTkFrame(self.root)
        button_frame.grid(row=2, column=0, padx=10, pady=(0, 10), sticky="ew")
        button_frame.grid_columnconfigure((0, 1, 2), weight=1)

        ctk.CTkButton(
            button_frame,
            text="Clear All",
            command=self.clear_history
        ).grid(row=0, column=0, padx=5, pady=5)
        
        ctk.CTkButton(
            button_frame,
            text="Settings",
            command=self.show_settings
        ).grid(row=0, column=1, padx=5, pady=5)
        
        self.pin_button = ctk.CTkButton(
            button_frame,
            text="Pin Window",
            command=self.toggle_pin
        )
        self.pin_button.grid(row=0, column=2, padx=5, pady=5)

    def create_clip_frame(self, item):
        clip_frame = ctk.CTkFrame(self.clips_frame)
        clip_frame.grid(sticky="ew", padx=5, pady=2)
        clip_frame.grid_columnconfigure(0, weight=1)
        
        content = item['content']
        content_preview = (content[:50] + "...") if len(content) > 50 else content
        
        label = ctk.CTkLabel(
            clip_frame,
            text=f"{item['timestamp']}\n{content_preview}",
            justify="left",
            anchor="w"
        )
        label.grid(row=0, column=0, padx=5, pady=5, sticky="w")
        
        pin_text = "Unpin" if item['pinned'] else "Pin"
        pin_btn = ctk.CTkButton(
            clip_frame,
            text=pin_text,
            width=60,
            command=lambda: self.toggle_clip_pin(content)
        )
        pin_btn.grid(row=0, column=1, padx=5, pady=5)
        
        copy_btn = ctk.CTkButton(
            clip_frame,
            text="Copy",
            width=60,
            command=lambda: self.copy_to_clipboard(content)
        )
        copy_btn.grid(row=0, column=2, padx=5, pady=5)
        
        return clip_frame

    def update_clips_display(self):
        # Clear existing clips
        for widget in self.clips_frame.winfo_children():
            widget.destroy()
        
        # Add clips from history
        for item in self.history_manager.get_history():
            self.create_clip_frame(item)

    def check_clipboard_updates(self):
        content = self.history_manager.clipboard_manager.check_for_new_content()
        if content:
            self.history_manager.add_to_history(content)
            self.update_clips_display()
        self.root.after(1000, self.check_clipboard_updates)

    def copy_to_clipboard(self, content):
        """Copy content to clipboard with error handling, feedback, and delay."""
        if not content:
            self.show_feedback("Nothing to copy", "warning")
            return False
        
        try:
            success = self.history_manager.clipboard_manager.set_clipboard_content(content)
            
            if success:
                self.show_feedback("Copied successfully!", "success")
                self.root.after(1000, self.hide_clipboard)
                return True
            else:
                self.show_feedback("Failed to copy content", "error")
                return False
                
        except Exception as e:
            print(f"Error copying to clipboard: {e}")
            self.show_feedback(f"Error copying: {str(e)}", "error")
            return False

    def show_feedback(self, message, type_="info"):
        """Show feedback message to user."""
        colors = {
            'success': ("green", "white"),
            'error': ("red", "white"),
            'warning': ("orange", "black"),
            'info': ("blue", "white")
        }
        
        fg_color, text_color = colors.get(type_, colors['info'])
        
        feedback = ctk.CTkLabel(
            self.root,
            text=message,
            fg_color=fg_color,
            text_color=text_color,
            corner_radius=8,
            padx=10,
            pady=5
        )
        feedback.place(relx=0.5, rely=0.9, anchor="center")
        self.root.after(2000, feedback.destroy)

    def toggle_clip_pin(self, content):
        self.history_manager.toggle_pin(content)
        self.update_clips_display()

    def filter_clips(self):
        search_text = self.search_var.get().lower()
        for widget in self.clips_frame.winfo_children():
            if isinstance(widget, ctk.CTkFrame):
                label = widget.winfo_children()[0]
                if search_text in label.cget("text").lower():
                    widget.grid()
                else:
                    widget.grid_remove()

    def clear_search(self):
        self.search_var.set("")
        self.filter_clips()

    def clear_history(self):
        self.history_manager.clear_history()
        self.update_clips_display()

    def toggle_pin(self):
        """Toggle window pin state and apply window constraints."""
        try:
            self.window_pinned = not self.window_pinned
            
            self.root.attributes('-topmost', self.window_pinned)
            
            if self.window_pinned:
                self.root.protocol("WM_DELETE_WINDOW", lambda: None)
                self.root.resizable(False, False)
                self.pin_button.configure(text="Unpin Window")
            else:
                self.root.protocol("WM_DELETE_WINDOW", self.hide_clipboard)
                self.root.resizable(True, True)
                self.pin_button.configure(text="Pin Window")
            
            self.settings['window_pinned'] = self.window_pinned
            self.save_settings()
            
        except Exception as e:
            print(f"Error in toggle_pin: {e}")
    
    def show_clipboard(self):
        """Shows the clipboard window"""
        self.update_clips_display()
        self.root.deiconify()
        self.root.lift()
        self.root.focus_force()
        self.clear_search()
        self.search_entry.focus_set()

    def hide_clipboard(self):
        """Hides the clipboard window instead of closing the application"""
        self.root.withdraw()
        self.clear_search()

    def load_settings(self):
        """Load settings with error handling."""
        self.settings = {}
        try:
            settings_dir = os.path.dirname(self.history_manager.history_file)
            settings_file = os.path.join(settings_dir, 'settings.json')
            os.makedirs(settings_dir, exist_ok=True)
            
            if os.path.exists(settings_file):
                try:
                    with open(settings_file, 'r', encoding='utf-8') as f:
                        self.settings = json.load(f)
                except json.JSONDecodeError:
                    print("Settings file is corrupted, using defaults")
                    corrupted_file = f"{settings_file}.corrupted"
                    os.rename(settings_file, corrupted_file)
                except Exception as e:
                    print(f"Error loading settings: {e}")
            
            self.apply_settings()
            
        except Exception as e:
            print(f"Error in load_settings: {e}")
            self.settings = {
                'theme': 'dark',
                'window_pinned': False
            }

    def apply_settings(self):
        """Apply settings with default values if needed."""
        try:
            theme = self.settings.get('theme', 'dark')
            self.change_theme(theme)
            
            if self.settings.get('window_pinned', False):
                self.window_pinned = True
                self.toggle_pin()
                
        except Exception as e:
            print(f"Error applying settings: {e}")

    def save_settings(self):
        settings_file = os.path.join(
            os.path.dirname(self.history_manager.history_file),
            'settings.json'
        )
        try:
            with open(settings_file, 'w') as f:
                json.dump(self.settings, f)
        except Exception as e:
            print(f"Error saving settings: {e}")

    def change_theme(self, theme):
        try:
            ctk.set_appearance_mode(theme)
            self.settings['theme'] = theme
            self.save_settings()
        except Exception as e:
            print(f"Error changing theme: {e}")

    def show_settings(self):
        settings_window = ctk.CTkToplevel(self.root)
        settings_window.title("Settings")
        settings_window.geometry("300x400")
        
        settings_window.update()
        
        x = self.root.winfo_x() + (self.root.winfo_width() - settings_window.winfo_width()) // 2
        y = self.root.winfo_y() + (self.root.winfo_height() - settings_window.winfo_height()) // 2
        settings_window.geometry(f"+{x}+{y}")
        
        settings_window.after(100, lambda: settings_window.grab_set())
        
        theme_label = ctk.CTkLabel(settings_window, text="Theme:")
        theme_label.pack(pady=(20, 5))
        
        theme_var = ctk.StringVar(value=self.settings.get('theme', 'dark'))
        theme_menu = ctk.CTkOptionMenu(
            settings_window,
            values=["light", "dark", "system"],
            variable=theme_var,
            command=lambda x: self.change_theme(x)
        )
        theme_menu.pack(pady=5)
        
        ctk.CTkButton(
            settings_window,
            text="Close",
            command=settings_window.destroy
        ).pack(pady=20)