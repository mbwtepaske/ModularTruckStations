"""Generates the mod's simple UI icons as PNG files.

Run: python Tools/generate_ui_icons.py
Output: Mods/ModularTruckStation/SourceArt/UI/*.png, to import into the editor with
Texture Group UI, Compression UserInterface2D and Mip Gen NoMipmaps.

The port button is two layers: T_MTS_PortFrame (background) and T_MTS_PortArrows (foreground). The arrows are a
2 x 2 atlas, columns Input | Output, rows normal | locked. Each cell maps onto the frame's opening (inside its
outline: frame pixels 52..204 x 44..192, i.e. 59.4% x 57.8% of the frame), so the widget shows one cell by
clipping the atlas to that window and slides the atlas to change state.

Shapes are drawn at SUPERSAMPLE times the final size and scaled down for smooth edges.
"""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

SIZE = 256
SUPERSAMPLE = 4
OUTPUT_DIR = Path(__file__).resolve().parent.parent / "Mods" / "ModularTruckStation" / "SourceArt" / "UI"

# Colors close to the vanilla UI and buildings.
BODY = (58, 61, 66, 255)
BODY_EDGE = (107, 112, 120, 255)
OPENING = (21, 23, 26, 255)
BOLT = (138, 144, 153, 255)
ACCENT = (250, 149, 73, 255)
ACCENT_DARK = (40, 42, 46, 255)

# Opening of the port frame, in frame pixels (left, top, right, bottom), and its outline width. The arrow cells
# fill the area inside the outline; the widget clips the atlas to that area.
OPENING_RECT = (48, 40, 208, 196)
OPENING_OUTLINE = 4
ARROW_WINDOW = tuple(v + OPENING_OUTLINE * sign for v, sign in zip(OPENING_RECT, (1, 1, -1, -1)))

# Arrow atlas cells: (column, row) -> direction and color. Input points right, output points left.
INPUT_COLOR = ACCENT
OUTPUT_COLOR = (120, 220, 120, 255)
LOCKED_COLOR = (150, 150, 150, 115)  # 45% opacity, so the locked arrows recede into the opening
ATLAS_CELLS = {
    (0, 0): ("right", INPUT_COLOR),
    (1, 0): ("left", OUTPUT_COLOR),
    (0, 1): ("right", LOCKED_COLOR),
    (1, 1): ("left", LOCKED_COLOR),
}
CELL_LABELS = {(0, 0): "Input", (1, 0): "Output", (0, 1): "Input, locked", (1, 1): "Output, locked"}

PREVIEW_BACKGROUND = (30, 30, 34, 255)


def canvas(width=SIZE, height=SIZE):
    image = Image.new("RGBA", (width * SUPERSAMPLE, height * SUPERSAMPLE), (0, 0, 0, 0))
    return image, ImageDraw.Draw(image)


def scaled(values):
    return [v * SUPERSAMPLE for v in values]


def save(image, name, size=(SIZE, SIZE)):
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUTPUT_DIR / name
    image.resize(size, Image.LANCZOS).save(path)
    print(f"wrote {path}")


def draw_round_line(draw, points, width, color):
    """Polyline with round joints and caps."""
    draw.line(points, fill=color, width=width, joint="curve")
    radius = width / 2
    for x, y in (points[0], points[-1]):
        draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=color)


def draw_chevrons(draw, origin, direction, color):
    """Three chevrons centered in a SIZE x SIZE cell at origin (cell pixels), pointing left or right."""
    stroke = 16 * SUPERSAMPLE
    chevron_width, chevron_height, spacing = 32, 84, 44
    total_width = chevron_width + 2 * spacing
    left = origin[0] + (SIZE - total_width) / 2
    top = origin[1] + (SIZE - chevron_height) / 2
    for index in range(3):
        x0 = left + index * spacing
        tip, back = (x0 + chevron_width, x0) if direction == "right" else (x0, x0 + chevron_width)
        points = [(back, top), (tip, top + chevron_height / 2), (back, top + chevron_height)]
        draw_round_line(draw, [tuple(scaled(p)) for p in points], stroke, color)


def port_arrows():
    """The 2 x 2 arrow atlas; each SIZE x SIZE cell maps onto the frame's opening."""
    image, draw = canvas(2 * SIZE, 2 * SIZE)
    for (column, row), (direction, color) in ATLAS_CELLS.items():
        draw_chevrons(draw, (column * SIZE, row * SIZE), direction, color)
    save(image, "T_MTS_PortArrows.png", (2 * SIZE, 2 * SIZE))


def port_frame():
    """Front view of a belt port: frame with bolts, dark opening for the arrows, hazard stripe at the bottom."""
    image, draw = canvas()
    s = SUPERSAMPLE
    draw.rounded_rectangle(scaled((12, 12, 244, 244)), radius=28 * s, fill=BODY, outline=BODY_EDGE, width=8 * s)

    # Hazard stripe band below the opening.
    band = Image.new("RGBA", image.size, (0, 0, 0, 0))
    band_draw = ImageDraw.Draw(band)
    band_top, band_bottom = 206, 228
    band_draw.rectangle(scaled((40, band_top, 216, band_bottom)), fill=ACCENT)
    for x in range(20, 236, 20):
        band_draw.polygon(
            [tuple(scaled(p)) for p in ((x, band_bottom), (x + 10, band_bottom), (x + 32, band_top), (x + 22, band_top))],
            fill=ACCENT_DARK,
        )
    mask = Image.new("L", image.size, 0)
    ImageDraw.Draw(mask).rectangle(scaled((40, band_top, 216, band_bottom)), fill=255)
    image.paste(band, (0, 0), mask)

    draw.rounded_rectangle(scaled(OPENING_RECT), radius=16 * s, fill=OPENING, outline=BODY_EDGE, width=OPENING_OUTLINE * s)

    for x, y in ((30, 30), (226, 30), (30, 226), (226, 226)):
        r = 7
        draw.ellipse(scaled((x - r, y - r, x + r, y + r)), fill=BOLT)
    save(image, "T_MTS_PortFrame.png")


def button(frame, atlas, offset):
    """The button as the widget shows it: the atlas at offset (in cells, may be fractional) clipped to the window."""
    left, top, right, bottom = ARROW_WINDOW
    width, height = right - left, bottom - top
    cell_atlas = atlas.resize((2 * width, 2 * height), Image.LANCZOS)
    x, y = round(offset[0] * width), round(offset[1] * height)
    result = frame.copy()
    result.alpha_composite(cell_atlas.crop((x, y, x + width, y + height)), (left, top))
    return result


def ease(t):
    """Smooth start and stop, like the widget's interpolation."""
    return t * t * (3 - 2 * t)


def preview():
    """Every port state, and an animation of the slides between them; for checking only, not imported."""
    frame = Image.open(OUTPUT_DIR / "T_MTS_PortFrame.png").convert("RGBA")
    atlas = Image.open(OUTPUT_DIR / "T_MTS_PortArrows.png").convert("RGBA")
    tile, size = 220, 120
    font = ImageFont.truetype("segoeui.ttf", 15)

    states = Image.new("RGBA", (tile * len(ATLAS_CELLS), 190), PREVIEW_BACKGROUND)
    draw = ImageDraw.Draw(states)
    for index, cell in enumerate(sorted(ATLAS_CELLS, key=lambda c: (c[1], c[0]))):
        image = button(frame, atlas, cell).resize((size, size), Image.LANCZOS)
        states.alpha_composite(image, (index * tile + (tile - size) // 2, 19))
        draw.text((index * tile + tile // 2, 169), CELL_LABELS[cell], font=font, fill=(205, 205, 210), anchor="mm")
    path = OUTPUT_DIR.parent / "Preview_PortButton_States.png"
    states.save(path)
    print(f"wrote {path}")

    # Input -> Output -> locked -> unlocked -> Input, each slide eased over 14 frames with a pause in between.
    path_cells = [(0, 0), (1, 0), (1, 1), (1, 0), (0, 0)]
    frames, durations = [], []
    for start, end in zip(path_cells, path_cells[1:]):
        for step in range(1, 15):
            t = ease(step / 14)
            offset = (start[0] + (end[0] - start[0]) * t, start[1] + (end[1] - start[1]) * t)
            image = Image.new("RGBA", (180, 180), PREVIEW_BACKGROUND)
            image.alpha_composite(button(frame, atlas, offset).resize((160, 160), Image.LANCZOS), (10, 10))
            frames.append(image.convert("RGB"))
            durations.append(33)
        durations[-1] = 700  # pause on the reached state
    path = OUTPUT_DIR.parent / "Preview_PortButton_Slide.gif"
    frames[0].save(path, save_all=True, append_images=frames[1:], duration=durations, loop=0)
    print(f"wrote {path}")


if __name__ == "__main__":
    port_arrows()
    port_frame()
    preview()
