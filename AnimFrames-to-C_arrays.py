import os
import glob
import re
from PIL import Image

INPUT_DIR = "frames/"
OUTPUT_FILE = "frames.h"

# 0xFE19 background
BACKGROUND_COLOR = (255, 194, 205, 255)

def rgb_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def extract_number(filename):
    numbers = re.findall(r'\d+', os.path.basename(filename))
    return int(numbers[0]) if numbers else 0

def generate_h_file():
    image_files = sorted(glob.glob(os.path.join(INPUT_DIR, "*.png")), key=extract_number)

    if not image_files:
        print("No PNG files found in the directory!")
        return

    print("Step 1: Scanning frames to calculate Global Bounding Box...")
    global_left = float('inf')
    global_top = float('inf')
    global_right = 0
    global_bottom = 0

    # PASS 1: Find the maximum bounds of the visible art across all frames
    for img_path in image_files:
        img = Image.open(img_path).convert("RGBA")
        # Extract only the Alpha channel to find where the actual drawing is
        alpha_channel = img.split()[3]
        bbox = alpha_channel.getbbox()

        if bbox: # If the frame isn't completely empty
            global_left = min(global_left, bbox[0])
            global_top = min(global_top, bbox[1])
            global_right = max(global_right, bbox[2])
            global_bottom = max(global_bottom, bbox[3])

    if global_left == float('inf'):
        print("Error: All images appear to be completely transparent/empty.")
        return

    anim_width = global_right - global_left
    anim_height = global_bottom - global_top
    total_frames = len(image_files)

    print(f" -> Found optimal bounds: X={global_left}, Y={global_top}, Width={anim_width}, Height={anim_height}")
    print(f" -> Total Frames: {total_frames}\n")

    crop_box = (global_left, global_top, global_right, global_bottom)

    print("Step 2: Processing and Compressing Frames...")
    with open(OUTPUT_FILE, "w") as f:
        f.write("#include <pgmspace.h>\n\n")

        # --- WRITE THE DYNAMIC METADATA TO THE HEADER ---
        f.write(f"// Automatically generated animation metadata\n")
        f.write(f"#define ANIM_X {global_left}\n")
        f.write(f"#define ANIM_Y {global_top}\n")
        f.write(f"#define ANIM_WIDTH {anim_width}\n")
        f.write(f"#define ANIM_HEIGHT {anim_height}\n")
        f.write(f"#define ANIM_TOTAL_FRAMES {total_frames}\n\n")

        frame_names = []

        # PASS 2: Crop, Background, and RLE Compress
        for idx, img_path in enumerate(image_files):
            img = Image.open(img_path).convert("RGBA")
            pink_bg = Image.new("RGBA", img.size, BACKGROUND_COLOR)
            blended_img = Image.alpha_composite(pink_bg, img)

            blended_img = blended_img.convert("RGB")
            # Crop using the dynamically generated box
            cropped_img = blended_img.crop(crop_box)

            pixels = list(cropped_img.getdata())

            rle_data = []
            current_color = rgb_to_rgb565(*pixels[0])
            run_count = 0

            for pixel in pixels:
                color = rgb_to_rgb565(*pixel)
                if color == current_color and run_count < 255:
                    run_count += 1
                else:
                    rle_data.append(run_count)
                    rle_data.append((current_color >> 8) & 0xFF)
                    rle_data.append(current_color & 0xFF)
                    current_color = color
                    run_count = 1

            rle_data.append(run_count)
            rle_data.append((current_color >> 8) & 0xFF)
            rle_data.append(current_color & 0xFF)

            array_name = f"frame_{idx}"
            frame_names.append(array_name)

            f.write(f"const uint8_t {array_name}[] PROGMEM = {{\n")
            for i in range(0, len(rle_data), 12):
                chunk = rle_data[i:i+12]
                f.write("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",\n")
            f.write("};\n\n")

        f.write("const uint8_t* const animation_frames[] PROGMEM = {\n")
        f.write("    " + ", ".join(frame_names) + "\n")
        f.write("};\n")

        print(f"\nDone! Saved to {OUTPUT_FILE}.")

if not os.path.exists(INPUT_DIR):
    os.makedirs(INPUT_DIR)
generate_h_file()
