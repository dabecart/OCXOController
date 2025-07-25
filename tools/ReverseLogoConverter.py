import re
import numpy as np
from PIL import Image

# === Settings ===
input_c_file = "output.c"
output_image_path = "reconstructed.png"
image_width = 16
tone_map = {
    0b00 : 0,
    0b01 : 122,
    0b10 : 195,
    0b11 : 255
}

def extract_u16_values_from_c(c_file_path):
    with open(c_file_path, "r") as file:
        content = file.read()
    # Match hex numbers like 0xABCD
    matches = re.findall(r"0x([0-9A-Fa-f]{1,4})", content)
    return [int(m, 16) for m in matches]

def decode_u16_to_pixels(u16_values):
    pixels = []
    for val in u16_values:
        for shift in range(0, 16, 2):
            bits = (val >> shift) & 0b11
            pixels.append(tone_map[bits])
    return pixels

def save_pixels_to_image(pixels, width, output_path):
    if len(pixels) % width != 0:
        raise ValueError("Pixel data length is not divisible by width")
    height = len(pixels) // width
    img_array = np.array(pixels, dtype=np.uint8).reshape((height, width))
    img = Image.fromarray(img_array, mode='L')
    img.save(output_path)
    print(f"Image saved to {output_path}")

if __name__ == "__main__":
    u16_data = extract_u16_values_from_c(input_c_file)
    pixel_values = decode_u16_to_pixels(u16_data)
    save_pixels_to_image(pixel_values, image_width, output_image_path)
