import re
import numpy as np
from PIL import Image

# === Settings ===
input_c_file = "output.c"
output_image_path = "reconstructed.png"
image_width = 160
tone_map = {
    0b00 : 0,
    0b01 : 122,
    0b10 : 195,
    0b11 : 255
}

def extract_u8_values_from_c(c_file_path):
    with open(c_file_path, "r") as file:
        content = file.read()
    # Match hex numbers like 0xABCD
    matches = re.findall(r"0x([0-9A-Fa-f]{1,2})", content)
    return [int(m, 16) for m in matches]

def decode_u8_to_pixels(u8_values):
    pixels = []
    for val in u8_values:
        color = tone_map[val >> 6]
        pixelCount = (val & 0x3F) + 1
        pixels.extend([color] * pixelCount)
    return pixels

def save_pixels_to_image(pixels, width, output_path):
    if len(pixels) % width != 0:
        raise ValueError(f"Pixel data length {len(pixels)} is not divisible by width")
    height = len(pixels) // width
    img_array = np.array(pixels, dtype=np.uint8).reshape((height, width))
    img = Image.fromarray(img_array, mode='L')
    img.save(output_path)
    print(f"Image saved to {output_path}")

if __name__ == "__main__":
    u16_data = extract_u8_values_from_c(input_c_file)
    pixel_values = decode_u8_to_pixels(u16_data)
    save_pixels_to_image(pixel_values, image_width, output_image_path)
