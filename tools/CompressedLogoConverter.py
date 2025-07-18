from PIL import Image
import numpy as np

# === Settings ===
input_image_path = "C:/Users/Daniel/repos/OCXOController/OCXOController_v2/Logo/OCXOLogoForTFT_BW.png"

def mapColor(value):
    if 0 <= value <= 36 :
        return 0b00
    elif value <= 110:
        return 0b01
    elif value <= 219:
        return 0b10
    elif value <= 255:
        return 0b11
    else:
        raise ValueError("invalid color")

def process_image(image_path):
    img = Image.open(image_path).convert("L")
    pixels = np.array(img)

    height, width = pixels.shape
    u8_data = []
    currentColorCount = -1
    currentColor = mapColor(pixels[0][0])

    for row in pixels:
        for i in range(width):
            if currentColor == -1:
                currentColor = mapColor(row[i])
            elif (mapColor(row[i]) != currentColor) or (currentColorCount >= 63):
                # Current color ended. Save it.
                u8_data.append((currentColor << 6) | currentColorCount)
                currentColor = mapColor(row[i])
                currentColorCount = 0
            else:
                currentColorCount += 1
    
    # Final append.
    u8_data.append((currentColor << 6) | currentColorCount)
    
    print(f"Compressed to {len(u8_data)} bytes")
    return u8_data, width, height

def format_for_c_array(data, width, height):
    c_lines = [
        f"#define LOGO_WIDTH {width}",
        f"#define LOGO_HEIGHT {height}",
        "const uint8_t OCXOLogo[] = {"
    ]
    line = "    "
    for i, val in enumerate(data):
        line += f"0x{val:02X}, "
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
