import os

from PIL import Image, ImageDraw

icon_size = 256
image = Image.new("RGBA", (icon_size, icon_size), (0, 0, 0, 0))
draw = ImageDraw.Draw(image)


padding = 20
draw.rounded_rectangle(
    [(padding, padding), (icon_size - padding, icon_size - padding)],
    radius=30,
    fill=(53, 59, 72, 255),
)


clipboard_width = 120
clipboard_height = 160
clipboard_x = (icon_size - clipboard_width) // 2
clipboard_y = (icon_size - clipboard_height) // 2


draw.rounded_rectangle(
    [(clipboard_x, clipboard_y), (clipboard_x + clipboard_width, clipboard_y + clipboard_height)],
    radius=10,
    fill=(236, 240, 241, 255),
)


clip_width = 50
clip_height = 20
clip_x = clipboard_x + (clipboard_width - clip_width) // 2
draw.rounded_rectangle(
    [(clip_x, clipboard_y - clip_height // 2), (clip_x + clip_width, clipboard_y + clip_height)],
    radius=5,
    fill=(52, 152, 219, 255),
)


line_padding = 15
line_height = 10
line_width_full = clipboard_width - 30
line_width_half = line_width_full // 2
line_color = (189, 195, 199, 255)
NUM_FULL_WIDTH_LINES = 3

for i in range(5):
    y_position = clipboard_y + 30 + i * (line_height + 10)
    line_width = line_width_full if i < NUM_FULL_WIDTH_LINES else line_width_half
    draw.rounded_rectangle(
        [(clipboard_x + 15, y_position), (clipboard_x + 15 + line_width, y_position + line_height)],
        radius=5,
        fill=line_color,
    )


if not os.path.exists("assets"):
    os.makedirs("assets")


image.save("assets/CopyClip_logo.png")

sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
icons = []
for size in sizes:
    icons.append(image.resize(size, Image.LANCZOS))


icons[0].save(
    "assets/CopyClip_logo.ico",
    sizes=[(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)],
    format="ICO",
)

print("Saved in assets/CopyClip_logo.png & assets/CopyClip_logo.ico")
