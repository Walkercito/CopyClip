# clipboard.py
import subprocess

class ClipboardManager:
    def __init__(self):
        self.current_clipboard = self.get_clipboard_content()
        
    def get_clipboard_content(self):
        """Gets the actual clipboard content using xsel"""
        try:
            output = subprocess.run(['xsel', '-b'], capture_output=True, text=True)
            return output.stdout
        except Exception as e:
            print(f"Error accessing clipboard: {e}")
            return ""
    
    def set_clipboard_content(self, content):
        """Sets a new clipboard content using xsel"""
        try:
            process = subprocess.Popen(['xsel', '-b', '-i'], stdin=subprocess.PIPE)
            process.communicate(input=content.encode())
            self.current_clipboard = content
        except Exception as e:
            print(f"Error setting clipboard: {e}")
    
    def check_for_new_content(self):
        """Checks if the clipboard content has changed"""
        content = self.get_clipboard_content()
        if content != self.current_clipboard:
            self.current_clipboard = content
            return content
        return None