# clipboard.py
import subprocess

class ClipboardManager:
    def __init__(self):
        """Initialize the clipboard manager using xsel."""
        try:
            subprocess.run(['which', 'xsel'], check=True, capture_output=True)
        except subprocess.CalledProcessError:
            raise RuntimeError("xsel is not installed. Please install it with: sudo apt-get install xsel")
        
        self.current_clipboard = self.get_clipboard_content()
        
    def get_clipboard_content(self):
        """Get the current content of the clipboard using xsel."""
        try:
            result = subprocess.run(['xsel', '-b', '-o'], capture_output=True, text=True)
            return result.stdout.strip()
        except Exception as e:
            print(f"Error getting clipboard content: {e}")
            return None
    
    def set_clipboard_content(self, content):
        """Sets a new clipboard content using xsel."""
        try:
            process = subprocess.Popen(['xsel', '-b', '-i'], stdin=subprocess.PIPE)
            process.communicate(input=content.encode())
            self.current_clipboard = content
            return True
        except Exception as e:
            print(f"Error setting clipboard: {e}")
            return False  
    
    def check_for_new_content(self):
        """Checks if the clipboard content has changed."""
        try:
            content = self.get_clipboard_content()
            if content is not None and content != self.current_clipboard:
                self.current_clipboard = content
                return content
        except Exception as e:
            print(f"Error checking clipboard content: {e}")
        return None