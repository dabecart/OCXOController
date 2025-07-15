from PIL import Image
import numpy as np

# === Settings ===
input_image_path = "C:/Users/Daniel/repos/OCXOController/OCXOController_v2/Logo/OCXOLogoForTFT_BW.png"

def map_pixel_to_2bit(value):
    if 0 <= value <= 10 :
        return 0b00
    elif 121 <=value <= 123:
        return 0b01
    elif 194 <= value <= 196:
        return 0b10
    elif 253 <= value <= 255:
        return 0b11
    else:
        raise ValueError("invalid color")

def process_image(image_path):
    img = Image.open(image_path).convert("L")
    pixels = np.array(img)

    height, width = pixels.shape
    if width % 8 != 0:
        raise ValueError("Image width must be divisible by 8")

    u16_data = []

    for row in pixels:
        for i in range(0, width, 8):
            byte_val = 0
            for j in reversed(range(8)):
                tone = map_pixel_to_2bit(row[i + j])
                byte_val = (byte_val << 2) | tone
            u16_data.append(byte_val)

    return u16_data, width, height

def format_for_c_array(data, width, height):
    c_lines = [
        f"#define LOGO_WIDTH {width}",
        f"#define LOGO_HEIGHT {height}",
        "const uint16_t OCXOLogo[] = {"
    ]
    line = "    "
    for i, val in enumerate(data):
        line += f"0x{val:04X}, "
        if (i + 1) % 8 == 0:
            c_lines.append(line)
            line = "    "
    if line.strip():
        c_lines.append(line)
    c_lines.append("};")
    return "\n".join(c_lines)

if __name__ == "__main__":
    u16_data, width, height = process_image(input_image_path)
    c_array_str = format_for_c_array(u16_data, width, height)
    with open("output.c", "w") as f:
        f.write(c_array_str)
    print("Conversion complete. Data saved to output.c")
